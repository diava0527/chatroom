#pragma once

#include <string>

#include "crow_all.h"

namespace chatroom::websocket {

class WsChatController {
public:
    virtual ~WsChatController() = default;

    // 1)代码逻辑：在 WebSocket 建连时校验 sessionId、建立用户与连接关系、同步在线状态，并准备聊天上下文。
    // 2)返回值类型：void，原因是 WebSocket 打开事件以连接初始化副作用为主，不返回独立业务对象，供 WebSocket 路由调用。
    // 3)参数类型：crow::websocket::connection& 与 const std::string& sessionId，原因是必须同时拿到连接对象和登录凭证完成身份识别，参数直接对应建连协议。
    virtual void OnOpen(crow::websocket::connection& connection, const std::string& sessionId) = 0;

    // 1)代码逻辑：接收客户端 WebSocket 原始消息，并交由分发模块解析成具体聊天事件处理。
    // 2)返回值类型：void，原因是消息处理结果通过连接回写或广播完成，不需要同步返回，供 WebSocket 路由调用。
    // 3)参数类型：crow::websocket::connection& 与 const std::string& rawMessage，原因是处理消息既依赖当前连接上下文也依赖原始报文内容，参数直接对应消息回调。
    virtual void OnMessage(crow::websocket::connection& connection, const std::string& rawMessage) = 0;

    // 1)代码逻辑：在 WebSocket 关闭时移除连接记录、更新在线用户状态，并触发必要的善后清理。
    // 2)返回值类型：void，原因是断连处理只需要执行资源清理动作，供 WebSocket 路由调用。
    // 3)参数类型：crow::websocket::connection&，原因是关闭事件以当前连接对象作为唯一定位入口，参数直接对应 Crow 回调。
    virtual void OnClose(crow::websocket::connection& connection) = 0;
};

}  // namespace chatroom::websocket
