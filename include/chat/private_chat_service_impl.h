#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "chat/private_chat_service.h"
#include "storage/private_chat_memory_store.h"

namespace chatroom::chat {

/// 私聊服务实现类：负责创建私聊窗口、发送私聊消息、拉取私聊历史以及清理用户相关会话
class PrivateChatServiceImpl : public PrivateChatService {
private:
    /// 私聊存储：通过依赖注入传入，本类只负责造数据与转发
    std::shared_ptr<storage::PrivateChatMemoryStore> store_;

    /// 自增计数器：用于生成全局唯一私聊消息 ID
    std::atomic<std::uint64_t> id_counter_{0};

    /// 生成全局唯一消息 ID（形如 "msg_1"、"msg_2"）
    std::string GenerateMessageId();

    /// 生成时间戳（格式 "YYYY-MM-DD HH:MM:SS"，定宽补零保证字符串比较等价于时间先后）
    std::string GenerateTimestamp() const;

public:
    /// 构造函数：注入私聊存储依赖
    explicit PrivateChatServiceImpl(std::shared_ptr<storage::PrivateChatMemoryStore> store);

    chatroom::models::PrivateChatSession CreatePrivateSession(
        const std::string& senderNickname, const std::string& receiverNickname) override;

    chatroom::models::Message SendPrivateMessage(
        const std::string& privateSessionId,
        const std::string& senderNickname,
        const std::string& content) override;

    std::vector<chatroom::models::Message> PullPrivateHistory(
        const std::string& privateSessionId) const override;

    void ClearSessionsByUser(const std::string& nickname) override;
};

}  // namespace chatroom::chat
