#include "storage/private_chat_memory_store_impl.h"

namespace chatroom::storage {

chatroom::models::PrivateChatSession PrivateChatMemoryStoreImpl::CreateSession(
    const std::string& senderNickname,
    const std::string& receiverNickname) {
    chatroom::models::PrivateChatSession session;
    session.privateSessionId = "private_" +
        std::to_string(nextSessionId_.fetch_add(1) + 1);
    session.senderNickname = senderNickname;
    session.receiverNickname = receiverNickname;

    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.emplace(session.privateSessionId, session);
    return session;
}

std::optional<chatroom::models::PrivateChatSession>
PrivateChatMemoryStoreImpl::FindSession(const std::string& privateSessionId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = sessions_.find(privateSessionId);
    return it == sessions_.end()
        ? std::nullopt
        : std::optional<chatroom::models::PrivateChatSession>(it->second);
}

bool PrivateChatMemoryStoreImpl::AppendMessage(
    const std::string& privateSessionId,
    const chatroom::models::Message& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = sessions_.find(privateSessionId);
    if (it == sessions_.end()) {
        return false;
    }

    it->second.messages.push_back(message);
    return true;
}

std::vector<chatroom::models::Message> PrivateChatMemoryStoreImpl::ListMessages(
    const std::string& privateSessionId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = sessions_.find(privateSessionId);
    return it == sessions_.end()
        ? std::vector<chatroom::models::Message>{}
        : it->second.messages;
}

void PrivateChatMemoryStoreImpl::RemoveSessionsByUser(const std::string& nickname) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = sessions_.begin(); it != sessions_.end();) {
        const auto& session = it->second;
        if (session.senderNickname == nickname ||
            session.receiverNickname == nickname) {
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
}

}  // namespace chatroom::storage
