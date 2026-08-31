#include "storage/lobby_memory_store_impl.h"

namespace chatroom::storage {

void LobbyMemoryStoreImpl::SaveLobbySession(
    const chatroom::models::LobbySession& session) {
    // 写入会话前加锁，防止多个用户同时进入大厅时竞争容器。
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.push_back(session);
}

<<<<<<< HEAD
=======
std::optional<std::string> LobbyMemoryStoreImpl::FindEnteredAt(
    const std::string& nickname) const {
    // 查询期间加锁，避免遍历会话时容器被其他线程修改。
    std::lock_guard<std::mutex> lock(mutex_);

    // 时间戳为固定宽度的 YYYY-MM-DD HH:MM:SS 格式，可直接进行字典序比较取最早。
    std::optional<std::string> earliest;
    for (const auto& session : sessions_) {
        if (session.nickname == nickname
            && (!earliest.has_value() || session.enteredAt < *earliest)) {
            earliest = session.enteredAt;
        }
    }
    return earliest;
}

>>>>>>> N-storage
void LobbyMemoryStoreImpl::AppendLobbyMessage(
    const chatroom::models::Message& message) {
    // 写入消息前加锁，确保消息容器保持一致状态。
    std::lock_guard<std::mutex> lock(mutex_);
    messages_.push_back(message);
}

std::vector<chatroom::models::Message> LobbyMemoryStoreImpl::ListMessagesAfter(
    const std::string& enteredAt) const {
    // 查询期间加锁，避免遍历消息时容器被其他线程修改。
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<chatroom::models::Message> visibleMessages;
    for (const auto& message : messages_) {
        // 时间戳为固定宽度的 YYYY-MM-DD HH:MM:SS 格式，可直接进行字典序比较。
        if (message.timestamp > enteredAt) {
            visibleMessages.push_back(message);
        }
    }
    return visibleMessages;
}

}  // namespace chatroom::storage
