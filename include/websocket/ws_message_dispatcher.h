#pragma once

#include <string>

#include "crow_all.h"

namespace chatroom::websocket {

class WsMessageDispatcher {
public:
    virtual ~WsMessageDispatcher() = default;

    // 1)代码逻辑：解析客户端 WebSocket 消息中的 event 字段，并按事件类型分发到大厅进入、大厅发言、历史拉取、私聊发送等具体流程。
    // 2)返回值类型：void，原因是分发结果通过后续控制器写回连接或广播，不需要额外同步返回值，供 WsChatController 调用。
    // 3)参数类型：crow::websocket::connection& 与 const std::string& rawMessage（JSON形式），原因是需要同时拿到当前连接上下文和原始消息文本做协议分发，参数直接对应 WebSocket 消息入口。
    virtual void Dispatch(crow::websocket::connection& connection, const std::string& rawMessage) = 0;
    //rawMessage各种格式在websocket_api_md中
};

}  // namespace chatroom::websocket
