#pragma once

#include <string>
#include <vector>

namespace chatroom::chat {

class OnlineUserService {
public:
    virtual ~OnlineUserService() = default;

    // 1)代码逻辑：在用户已登录且 WebSocket 已连接后把该用户记为在线用户。
    // 2)返回值类型：void，原因是这里只定义在线状态登记动作，供 WebSocket 建连流程调用。
    // 3)参数类型：const std::string& nickname，原因是在线用户在当前系统中以唯一昵称为主标识，参数直接对应在线用户定义。
    virtual void MarkOnline(const std::string& nickname) = 0;

    // 1)代码逻辑：在用户断开连接或登出时移除在线状态，保证在线列表只包含有效在线用户。
    // 2)返回值类型：void，原因是这里只描述离线清理动作，供 WebSocket 断连流程和登出流程调用。
    // 3)参数类型：const std::string& nickname，原因是在线状态按昵称维度维护，参数直接对应在线用户定义。
    virtual void MarkOffline(const std::string& nickname) = 0;

    // 1)代码逻辑：返回当前在线用户昵称列表，供前端展示在线可私聊对象。
    // 2)返回值类型：std::vector<std::string>，原因是前端展示在线列表天然需要数组结构，供 HTTP 在线用户接口和在线变更广播接口调用。
    // 3)参数类型：无参数，原因是该接口返回的是系统当前全量在线视图。
    virtual std::vector<std::string> ListOnlineUsers() const = 0;
};

}  // namespace chatroom::chat
