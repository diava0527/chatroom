// 私聊服务实现：对应 include/chat/private_chat_service.h 接口。
// 当前仅搭好骨架，方法体为占位实现，具体业务逻辑待补充。
#include "chat/private_chat_service.h"

namespace chatroom::chat {

// 私聊服务的具体实现类。
class PrivateChatServiceImpl : public PrivateChatService {
public:
    // 创建私聊窗口：建立双方用户之间的当前私聊会话。
    chatroom::models::PrivateChatSession CreatePrivateSession(
        const std::string& senderNickname,
        const std::string& receiverNickname) override {
        // TODO: 实现创建私聊窗口逻辑
        (void)senderNickname;
        (void)receiverNickname;
        return {};
    }

    // 发送私聊消息：向指定窗口写入消息，返回完整消息体。
    chatroom::models::Message SendPrivateMessage(
        const std::string& privateSessionId,
        const std::string& senderNickname,
        const std::string& content) override {
        // TODO: 实现发送私聊消息逻辑
        (void)privateSessionId;
        (void)senderNickname;
        (void)content;
        return {};
    }

    // 拉取私聊历史：返回指定窗口的全部消息。
    std::vector<chatroom::models::Message> PullPrivateHistory(
        const std::string& privateSessionId) const override {
        // TODO: 实现私聊历史拉取逻辑
        (void)privateSessionId;
        return {};
    }

    // 清理用户会话：删除与该用户相关的全部私聊窗口。
    void ClearSessionsByUser(const std::string& nickname) override {
        // TODO: 实现清理用户私聊窗口逻辑
        (void)nickname;
    }
};

}  // namespace chatroom::chat
