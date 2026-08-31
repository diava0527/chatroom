#include "storage/session_memory_store_impl.h"

namespace chatroom::storage {

void SessionMemoryStoreImpl::SaveSession(const std::string& sessionId,
                                         const std::string& nickname) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_[sessionId] = nickname;
}

std::optional<std::string> SessionMemoryStoreImpl::FindNicknameBySessionId(
    const std::string& sessionId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = sessions_.find(sessionId);
    return it == sessions_.end() ? std::nullopt
                                 : std::optional<std::string>(it->second);
}

void SessionMemoryStoreImpl::RemoveSession(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(sessionId);
}

}  // namespace chatroom::storage
