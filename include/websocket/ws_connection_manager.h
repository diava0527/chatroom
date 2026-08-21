#pragma once

#include <string>
#include <vector>

#include "crow_all.h"

namespace chatroom::websocket {

class WsConnectionManager {
public:
    virtual ~WsConnectionManager() = default;

    // 1)代码逻辑：记录用户昵称与 WebSocket 连接对象的关联关系，用于后续单播、广播和在线状态维护。
    // 2)返回值类型：void，原因是该接口只执行连接登记动作，不需要直接返回业务结果，供 WebSocket 建连接口调用。
    // 3)参数类型：const std::string& 与 crow::websocket::connection&，原因是必须同时知道是哪个用户上线以及对应哪个连接对象，参数直接对应连接管理要求。
    virtual void BindConnection(const std::string& nickname, crow::websocket::connection& connection) = 0;

    // 1)代码逻辑：在连接断开时解除昵称与连接对象的绑定，防止后续继续向失效连接发消息。
    // 2)返回值类型：void，原因是这里只定义断连清理动作，供 WebSocket 关闭接口调用。
    // 3)参数类型：crow::websocket::connection&，原因是断连时最直接能拿到的是连接对象本身，参数直接对应 Crow WebSocket 关闭回调。
    virtual void UnbindConnection(crow::websocket::connection& connection) = 0;

    // 1)代码逻辑：向指定用户对应的连接发送一条 WebSocket 消息，用于私聊消息和状态消息单播。
    // 2)返回值类型：bool，原因是需要表达目标用户当前是否存在可发送连接，供私聊消息发送接口调用。
    // 3)参数类型：const std::string& nickname 与 const std::string& payload，原因是单播必须明确发送目标和发送内容，参数直接对应实时通信需求。
    virtual bool SendToUser(const std::string& nickname, const std::string& payload) = 0;

    // 1)代码逻辑：向所有在线连接广播一条 WebSocket 消息，用于大厅消息和在线用户变化通知。
    // 2)返回值类型：void，原因是广播关注的是发送动作本身，不要求逐个返回结果，供大厅聊天和在线列表变更接口调用。
    // 3)参数类型：const std::string& payload，原因是广播消息面向所有在线连接，只需要统一消息内容。
    virtual void Broadcast(const std::string& payload) = 0;

    // 1)代码逻辑：返回当前已建立 WebSocket 连接的在线用户昵称列表。
    // 2)返回值类型：std::vector<std::string>，原因是前端在线用户展示需要昵称数组，供在线用户模块和广播模块调用。
    // 3)参数类型：无参数，原因是这里读取的是当前全量在线连接状态。
    virtual std::vector<std::string> ListConnectedUsers() const = 0;
};

}  // namespace chatroom::websocket
