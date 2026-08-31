#include "auth_service_impl.h"

#include <utility>
#include "common/random_id.h"

namespace chatroom::user {

AuthServiceImpl::AuthServiceImpl(std::shared_ptr<storage::UserMemoryStore> store,
                                 SessionEnded onSessionEnded, Now now)
    : user_store(std::move(store)), onSessionEnded_(std::move(onSessionEnded)), now_(std::move(now)) {}

// 1)代码逻辑：向存储注册唯一昵称及密码，沿用现有账号数据格式。
// 2)返回值类型：bool，供注册控制器判断是否成功。
// 3)参数类型：昵称和密码字符串对应客户端注册字段。
bool AuthServiceImpl::Register(const std::string& nickname, const std::string& password) {
    chatroom::models::User user;
    user.nickname = nickname;
    user.password = password;
    return user_store->SaveUser(user);
}

bool AuthServiceImpl::IsExpired(const Session& session) const {
    if (session.connection != nullptr) return false;
    // 登录后预留 15 秒建立首个连接；已连过的会话预留 5 分钟供刷新/断网/切后台重连。
    const auto grace = session.wasConnected ? std::chrono::seconds(300) : std::chrono::seconds(15);
    return now_() - session.disconnectedAt >= grace;
}

// 调用方持有 session_mtx。清理失败时保留映射供下次重试，避免删除新会话的私聊。
void AuthServiceImpl::RemoveSession(const std::string& sessionId) {
    const auto found = session_map.find(sessionId);
    if (found == session_map.end()) return;
    const std::string nickname = found->second.nickname;
    if (onSessionEnded_) onSessionEnded_(nickname);
    nickname_map.erase(nickname);
    session_map.erase(found);
}

// 1)代码逻辑：先校验密码；拒绝仍在线的重复登录，离线用户可立即重新登录并废弃旧会话。
// 2)返回值类型：std::optional<std::string>，供 HTTP 控制器返回新会话或登录失败。
// 3)参数类型：昵称和密码必须来自本次登录请求，断连不绕过密码校验。
std::optional<std::string> AuthServiceImpl::Login(const std::string& nickname, const std::string& password) {
    std::lock_guard<std::mutex> lock(session_mtx);
    const auto user = user_store->FindUserByNickname(nickname);
    if (!user || user->password != password) return std::nullopt;

    const auto existing = nickname_map.find(nickname);
    if (existing != nickname_map.end()) {
        const auto& session = session_map.at(existing->second);
        if (session.connection != nullptr || (!session.wasConnected && !IsExpired(session))) {
            return std::nullopt;
        }
        const std::string previousId = existing->second;
        RemoveSession(previousId);
    }
    std::string sessionId;
    do { sessionId = common::GenerateRandomHexId(); } while (session_map.count(sessionId));
    session_map.emplace(sessionId, Session{nickname, nullptr, false, now_()});
    nickname_map.emplace(nickname, sessionId);
    return sessionId;
}

// 1)代码逻辑：显式退出时立即撤销凭据并统一清理私聊。
// 2)返回值类型：bool，供退出接口返回成功或无效会话。
// 3)参数类型：sessionId 唯一标识要退出的会话，不能按昵称误删新登录。
bool AuthServiceImpl::Logout(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(session_mtx);
    if (!session_map.count(sessionId)) return false;
    RemoveSession(sessionId);
    return true;
}

// 1)代码逻辑：验证会话存在且未超过断连宽限期，不因 HTTP 请求延长断连期限。
// 2)返回值类型：std::optional<std::string>，供 HTTP 和 WebSocket 握手鉴权。
// 3)参数类型：sessionId 为客户端持有的登录凭据。
std::optional<std::string> AuthServiceImpl::ValidateSession(const std::string& sessionId) const {
    std::lock_guard<std::mutex> lock(session_mtx);
    const auto found = session_map.find(sessionId);
    if (found == session_map.end() || IsExpired(found->second)) return std::nullopt;
    return found->second.nickname;
}

std::optional<std::string> AuthServiceImpl::AttachConnection(const std::string& sessionId, const void* connection) {
    std::lock_guard<std::mutex> lock(session_mtx);
    const auto found = session_map.find(sessionId);
    if (!connection || found == session_map.end() || IsExpired(found->second)) return std::nullopt;
    found->second.connection = connection;
    found->second.wasConnected = true;
    return found->second.nickname;
}

void AuthServiceImpl::DetachConnection(const std::string& sessionId, const void* connection) {
    std::lock_guard<std::mutex> lock(session_mtx);
    const auto found = session_map.find(sessionId);
    if (found == session_map.end() || !connection || found->second.connection != connection) return;
    found->second.connection = nullptr;
    found->second.disconnectedAt = now_();
}

bool AuthServiceImpl::ValidateConnection(const std::string& sessionId, const void* connection) const {
    std::lock_guard<std::mutex> lock(session_mtx);
    const auto found = session_map.find(sessionId);
    return connection && found != session_map.end() && found->second.connection == connection;
}

void AuthServiceImpl::ExpireDisconnectedSessions() {
    std::lock_guard<std::mutex> lock(session_mtx);
    for (auto it = session_map.begin(); it != session_map.end();) {
        if (IsExpired(it->second)) {
            const std::string id = it->first;
            ++it;
            RemoveSession(id);
        } else ++it;
    }
}

}  // namespace chatroom::user
