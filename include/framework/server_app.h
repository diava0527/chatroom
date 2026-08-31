#pragma once

#include "framework/app.h"

namespace chatroom::framework {

class ServerApp {
public:
    virtual ~ServerApp() = default;

    // 1)代码逻辑：统一创建和持有 Crow 应用实例，组织路由注册、模块装配和服务器启动顺序。
    // 2)返回值类型：void，原因是该接口本质上是服务启动动作，不需要向其他接口返回业务结果，供 main 函数调用。
    // 3)参数类型：无参数，原因是当前架构阶段只描述启动流程，不要求外部传入额外业务参数。
    virtual void Run() = 0;

    // 1)代码逻辑：返回当前服务器内部持有的 Crow 应用实例，供路由注册模块完成 HTTP 和 WebSocket 挂载。
    // 2)返回值类型：crow::SimpleApp&，原因是路由注册必须直接操作应用对象，供 RouterRegistry 调用。
    // 3)参数类型：无参数，原因是这里只是读取当前服务对象内部状态，不依赖外部输入。
    virtual ChatroomApp& GetApp() = 0;
};

}  // namespace chatroom::framework
