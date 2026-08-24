#pragma once

#include <optional>
#include <string>
#include <vector>

#include "models/message.h"
#include "models/private_chat_session.h"

namespace chatroom::storage {

class PrivateChatMemoryStore {
public:
    virtual ~PrivateChatMemoryStore() = default;

    // 1)代码逻辑：创建新的当前私聊窗口会话，并把会话放入 map 结构中统一管理。
    // 2)返回值类型：chatroom::models::PrivateChatSession，原因是创建后上层需要拿到完整会话对象继续响应前端，供创建私聊窗口接口调用。
    // 3)参数类型：const std::string& senderNickname 与 const std::string& receiverNickname，原因是当前私聊窗口必须明确双方参与者，参数直接对应私聊窗口需求。
    virtual chatroom::models::PrivateChatSession CreateSession(
        const std::string& senderNickname,
        const std::string& receiverNickname) = 0;

    // 1)代码逻辑：根据私聊窗口会话标识查询当前会话，用于发送消息和拉取会话历史。
    // 2)返回值类型：std::optional<chatroom::models::PrivateChatSession>，原因是指定会话可能存在也可能不存在，供私聊消息接口和历史接口调用。
    // 3)参数类型：const std::string& privateSessionId，原因是当前私聊历史完全以窗口会话为边界，参数直接对应私聊 map 键值设计。
    virtual std::optional<chatroom::models::PrivateChatSession> FindSession(const std::string& privateSessionId) const = 0;

    // 1)代码逻辑：向指定当前私聊窗口会话中追加消息，保证消息只记录在该窗口名下。
    // 2)返回值类型：bool，原因是需要表达目标窗口是否存在，供私聊消息发送接口调用。
    // 3)参数类型：const std::string& privateSessionId 与 const chatroom::models::Message&，原因是追加消息必须同时定位窗口和拿到完整消息体，参数直接对应“私聊记录使用 map”的实现方式。
    virtual bool AppendMessage(const std::string& privateSessionId, const chatroom::models::Message& message) = 0;

    // 1)代码逻辑：返回指定私聊窗口会话的全部消息历史，只包含该窗口建立后的记录。
    // 2)返回值类型：std::vector<chatroom::models::Message>，原因是私聊历史前端需要按列表渲染，供私聊历史接口调用。
    // 3)参数类型：const std::string& privateSessionId，原因是当前私聊历史边界由窗口会话唯一确定，参数直接对应需求。
    virtual std::vector<chatroom::models::Message> ListMessages(const std::string& privateSessionId) const = 0;

    // 1)代码逻辑：在用户登出后删除与该用户有关的全部私聊窗口会话和对应消息记录。
    // 2)返回值类型：void，原因是这里只定义清理动作，不需要返回单独业务数据，供登出流程调用。
    // 3)参数类型：const std::string& nickname，原因是清理范围围绕某个用户展开，参数直接对应登出后的私聊清空要求。
    virtual void RemoveSessionsByUser(const std::string& nickname) = 0;
};

}  // namespace chatroom::storage
