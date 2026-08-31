#pragma once

#include "framework/app.h"

namespace chatroom::framework {

class RouterRegistry {
public:
    virtual ~RouterRegistry() = default;

    // 1)代码逻辑：集中注册所有 HTTP 路由和 WebSocket 路由，统一维护接口路径与控制器绑定关系。
    // 2)返回值类型：void，原因是路由注册属于启动装配动作，不需要返回单独业务数据，供 ServerApp 调用。
    // 3)参数类型：crow::SimpleApp&，原因是 Crow 的所有路由都必须绑定到具体应用实例上，参数直接对应框架注册需求。
    virtual void RegisterAll(ChatroomApp& app) = 0;
};

}  // namespace chatroom::framework
