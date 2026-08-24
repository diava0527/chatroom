// 大厅聊天服务实现：对应 include/chat/lobby_service_impl.h 头文件
#include "chat/lobby_service_impl.h"
#include "models/lobby_session.h"
#include "common/constants.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>

namespace chatroom::chat {

// 生成全局唯一消息 ID：形如 "msg_1"、"msg_2" 等
std::string LobbyServiceImpl::GenerateMessageId() {
    return "msg_" + std::to_string(id_counter_.fetch_add(1) + 1);
}

// 生成时间戳：格式为 "YYYY-MM-DD HH:MM:SS"，每个字段定宽补零，字符串比较等价于时间先后
std::string LobbyServiceImpl::GenerateTimestamp() const {
    using namespace std::chrono;
    std::time_t t = system_clock::to_time_t(system_clock::now());
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// 构造函数：注入大厅存储依赖
LobbyServiceImpl::LobbyServiceImpl(std::shared_ptr<storage::LobbyMemoryStore> store)
    : store_(std::move(store)) {
}

// 进入大厅：记录用户进入时间，作为历史可见边界
std::string LobbyServiceImpl::EnterLobby(const std::string& nickname) {
    std::string enteredAt = GenerateTimestamp();
    chatroom::models::LobbySession session;
    session.nickname = nickname;
    session.enteredAt = enteredAt;
    store_->SaveLobbySession(session);
    return enteredAt;
}

// 发送大厅消息：构造并保存完整消息，返回完整消息体
chatroom::models::Message LobbyServiceImpl::SendLobbyMessage(
    const std::string& senderNickname,
    const std::string& content) {
    chatroom::models::Message msg;
    msg.messageId = GenerateMessageId();
    msg.senderNickname = senderNickname;
    // 大厅消息没有具体接收者，用 "LOBBY" 标记广播
    msg.receiverNickname = std::string(chatroom::common::kLobbyReceiver);
    msg.content = content;
    msg.timestamp = GenerateTimestamp();
    store_->AppendLobbyMessage(msg);
    return msg;
}

// 拉取可见历史：返回进入时间之后的消息列表
std::vector<chatroom::models::Message> LobbyServiceImpl::PullVisibleHistory(
    const std::string& enteredAt) const {
    return store_->ListMessagesAfter(enteredAt);
}

}  // namespace chatroom::chat
