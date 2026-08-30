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
