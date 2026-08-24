#pragma once

#include <string>
#include <vector>

#include "models/message.h"
#include "models/private_chat_session.h"

namespace chatroom::chat {

class PrivateChatService {
public:
    virtual ~PrivateChatService() = default;

    // 1)代码逻辑：由发起用户主动创建当前私聊窗口会话，并为该窗口准备独立消息记录容器。
    // 2)返回值类型：chatroom::models::PrivateChatSession，原因是前端创建成功后需要立即拿到会话标识和会话双方信息，供创建私聊窗口 HTTP 接口调用。
    // 3)参数类型：const std::string& senderNickname 与 const std::string& receiverNickname，原因是当前私聊窗口天然由双方用户共同定义，参数直接对应私聊会话开启方式 A。
    virtual chatroom::models::PrivateChatSession CreatePrivateSession(
        const std::string& senderNickname,
        const std::string& receiverNickname) = 0;

    // 1)代码逻辑：向指定当前私聊窗口写入一条私聊消息，并返回完整消息体供双方实时收发。
    // 2)返回值类型：chatroom::models::Message，原因是 WebSocket 推送私聊消息需要统一消息模型，供私聊发送消息接口调用。
    // 3)参数类型：const std::string& privateSessionId、const std::string& senderNickname、const std::string& content，原因是私聊发送必须同时明确属于哪个窗口、由谁发送、发送什么内容，参数直接对应私聊业务需求。
    virtual chatroom::models::Message SendPrivateMessage(
        const std::string& privateSessionId,
        const std::string& senderNickname,
        const std::string& content) = 0;

    // 1)代码逻辑：返回指定当前私聊窗口会话中的所有消息，只包含该窗口开启后的记录。
    // 2)返回值类型：std::vector<chatroom::models::Message>，原因是前端私聊历史需要顺序渲染消息列表，供私聊历史接口调用。
    // 3)参数类型：const std::string& privateSessionId，原因是私聊历史边界由当前窗口会话唯一确定，参数直接对应私聊记录存储方式。
    virtual std::vector<chatroom::models::Message> PullPrivateHistory(const std::string& privateSessionId) const = 0;

    // 1)代码逻辑：在用户退出登录时清空与该用户有关的全部当前私聊窗口会话和消息历史。
    // 2)返回值类型：void，原因是该接口只负责登出后的清理动作，不需要返回业务对象，供登出流程调用。
    // 3)参数类型：const std::string& nickname，原因是清理范围围绕指定用户展开，参数直接对应“退出登录后清空和用户有关的私聊窗口”要求。
    virtual void ClearSessionsByUser(const std::string& nickname) = 0;
};

}  // namespace chatroom::chat
