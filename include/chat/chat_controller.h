#pragma once

#include "crow_all.h"

namespace chatroom::chat {

class ChatController {
public:
    virtual ~ChatController() = default;

    // 1)代码逻辑：接收创建私聊窗口 HTTP 请求，校验 sessionId 和目标用户后创建新的当前私聊会话。
    // 2)返回值类型：crow::response，原因是创建私聊窗口是标准 HTTP 接口，供 /api/v1/private-chat/session 路由调用。
    // 3)参数类型：const crow::request&，原因是 sessionId 和目标昵称都从 HTTP 请求中获取，参数直接对应 Crow 路由签名。
    virtual crow::response CreatePrivateSession(const crow::request& request) = 0;

    // 1)代码逻辑：接收在线用户列表 HTTP 请求，校验身份后返回当前在线昵称集合。
    // 2)返回值类型：crow::response，原因是在线用户列表需要通过 HTTP 返回给前端，供 /api/v1/users/online 路由调用。
    // 3)参数类型：const crow::request&，原因是鉴权所需的 sessionId 来自请求头，参数直接对应 Crow 路由签名。
    virtual crow::response ListOnlineUsers(const crow::request& request) = 0;
};

}  // namespace chatroom::chat
