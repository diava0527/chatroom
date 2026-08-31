#include "framework/crow_server_app.h"

#include <utility>

namespace chatroom::framework {

CrowServerApp::CrowServerApp(RouterRegistry& routerRegistry,
                             std::uint16_t port,
                             std::string bindAddress,
                             std::string frontendOrigin)
    : routerRegistry_(routerRegistry),
      port_(port),
      bindAddress_(std::move(bindAddress)),
      frontendOrigin_(std::move(frontendOrigin)) {}

void CrowServerApp::Run() {
    auto& cors = app_.get_middleware<crow::CORSHandler>();
    cors.global()
        .origin(frontendOrigin_)
        .methods(crow::HTTPMethod::Get,
                 crow::HTTPMethod::Post,
                 crow::HTTPMethod::Options)
        .headers("Content-Type", "X-Session-Id")
        .max_age(600);

    routerRegistry_.RegisterAll(app_);
    app_.bindaddr(bindAddress_).port(port_).multithreaded().run();
}

ChatroomApp& CrowServerApp::GetApp() {
    return app_;
}

}  // namespace chatroom::framework
