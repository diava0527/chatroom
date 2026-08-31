#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include "auth_service.h"
#include "storage/user_memory_store.h"

namespace chatroom::user {

class AuthServiceImpl : public AuthService {
public:
    using Clock = std::chrono::steady_clock;
    using Now = std::function<Clock::time_point()>;
    using SessionEnded = std::function<void(const std::string&)>;

    explicit AuthServiceImpl(std::shared_ptr<storage::UserMemoryStore> store,
                             SessionEnded onSessionEnded = {},
                             Now now = [] { return std::chrono::steady_clock::now(); });

    bool Register(const std::string& nickname, const std::string& password) override;
    std::optional<std::string> Login(const std::string& nickname, const std::string& password) override;
    bool Logout(const std::string& sessionId) override;
    std::optional<std::string> ValidateSession(const std::string& sessionId) const override;
    std::optional<std::string> AttachConnection(const std::string& sessionId, const void* connection) override;
    void DetachConnection(const std::string& sessionId, const void* connection) override;
    bool ValidateConnection(const std::string& sessionId, const void* connection) const override;

    // 1)代码逻辑：回收超时的未连接会话，清理私聊并解除昵称占用。
    // 2)返回值类型：void，供服务器周期任务调用。
    // 3)参数类型：无参数，使用构造时注入的单调时钟，测试无需真实等待。
    void ExpireDisconnectedSessions();

private:
    struct Session {
        std::string nickname;
        const void* connection = nullptr;
        bool wasConnected = false;
        Clock::time_point disconnectedAt;
    };
    bool IsExpired(const Session& session) const;
    void RemoveSession(const std::string& sessionId);

    std::shared_ptr<storage::UserMemoryStore> user_store;
    std::unordered_map<std::string, Session> session_map;
    std::unordered_map<std::string, std::string> nickname_map;
    mutable std::mutex session_mtx;
    SessionEnded onSessionEnded_;
    Now now_;
};

}  // namespace chatroom::user
