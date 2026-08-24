#pragma once

#include <cstdint>

#include "framework/router_registry.h"
#include "framework/server_app.h"

namespace chatroom::framework {

class CrowServerApp final : public ServerApp {
public:
    // 1)代码逻辑：创建并持有 Crow 应用实例，同时接入统一路由注册中心和监听端口配置，作为后端启动容器。
    // 2)返回值类型：无返回值，原因是构造阶段只负责完成对象初始化，供 main 函数创建服务实例。
    // 3)参数类型：RouterRegistry& 与 std::uint16_t，原因是服务启动必须依赖外部路由装配器和监听端口，这两个参数正好覆盖基础启动条件。
    explicit CrowServerApp(RouterRegistry& routerRegistry, std::uint16_t port = 18080);

    // 1)代码逻辑：先触发全部路由装配，再启动 Crow HTTP/WebSocket 服务监听指定端口。
    // 2)返回值类型：void，原因是服务器启动属于阻塞式运行流程，不向上层返回业务对象，供 main 函数调用。
    // 3)参数类型：无参数，原因是运行所需的应用实例、路由中心和端口都已在构造阶段注入。
    void Run() override;

    // 1)代码逻辑：返回当前服务内部持有的 Crow 应用实例，供装配阶段或测试阶段访问同一应用上下文。
    // 2)返回值类型：crow::SimpleApp&，原因是调用方需要直接操作实际应用对象完成注册或检查。
    // 3)参数类型：无参数，原因是这里只读取当前对象内部状态，不依赖外部输入。
    crow::SimpleApp& GetApp() override;

private:
    crow::SimpleApp app_;
    RouterRegistry& routerRegistry_;
    std::uint16_t port_;
};

}  // namespace chatroom::framework
