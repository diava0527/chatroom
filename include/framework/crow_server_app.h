#pragma once

#include <cstdint>
#include <string>

#include "framework/router_registry.h"
#include "framework/server_app.h"

namespace chatroom::framework {

class CrowServerApp final : public ServerApp {
public:
    CrowServerApp(RouterRegistry& routerRegistry,
                  std::uint16_t port = 8080,
                  std::string bindAddress = "127.0.0.1",
                  std::string frontendOrigin = "http://localhost:5500");

    // 1)代码逻辑：先触发全部路由装配，再启动 Crow HTTP/WebSocket 服务监听指定端口�?
    // 2)返回值类型：void，原因是服务器启动属于阻塞式运行流程，不向上层返回业务对象，�?main 函数调用�?
    // 3)参数类型：无参数，原因是运行所需的应用实例、路由中心和端口都已在构造阶段注入�?
    void Run() override;
    ChatroomApp& GetApp() override;

private:
    ChatroomApp app_;
    RouterRegistry& routerRegistry_;
    std::uint16_t port_;
    std::string bindAddress_;
    std::string frontendOrigin_;
};

}  // namespace chatroom::framework
