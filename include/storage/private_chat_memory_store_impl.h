#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "storage/private_chat_memory_store.h"

namespace chatroom::storage {

/// 私聊窗口存储的内存实现：以私聊会话 ID 为键保存会话及其消息。
class PrivateChatMemoryStoreImpl final : public PrivateChatMemoryStore {
public:
    /// 创建双方用户的私聊窗口，并返回包含唯一会话 ID 的完整会话对象。
    chatroom::models::PrivateChatSession CreateSession(
        const std::string& senderNickname,
        const std::string& receiverNickname) override;

    /// 按会话 ID 查找私聊窗口；不存在时返回 std::nullopt。
    std::optional<chatroom::models::PrivateChatSession> FindSession(
        const std::string& privateSessionId) const override;

    /// 向目标窗口写入一条消息；仅当窗口存在时返回 true。
    bool AppendMessage(const std::string& privateSessionId,
                       const chatroom::models::Message& message) override;

    /// 返回目标窗口的消息副本；窗口不存在时返回空列表。
    std::vector<chatroom::models::Message> ListMessages(
        const std::string& privateSessionId) const override;

    /// 删除 nickname 作为任一参与者的所有私聊窗口及其消息。
    void RemoveSessionsByUser(const std::string& nickname) override;

private:
    /// 生成唯一私聊窗口 ID，原子递增支持并发创建。
    std::atomic<std::uint64_t> nextSessionId_{0};

    /// 保护 sessions_，支持 WebSocket 请求并发读写。
    mutable std::mutex mutex_;

    /// 私聊会话及其消息，以 privateSessionId 为键管理。
    std::unordered_map<std::string, chatroom::models::PrivateChatSession> sessions_;
};

}  // namespace chatroom::storage
