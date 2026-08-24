#include "framework/crow_server_app.h"

namespace chatroom::framework {

CrowServerApp::CrowServerApp(RouterRegistry& routerRegistry, std::uint16_t port)
    : routerRegistry_(routerRegistry), port_(port) {}

void CrowServerApp::Run() {
    routerRegistry_.RegisterAll(app_);
    app_.port(port_).multithreaded().run();
}

crow::SimpleApp& CrowServerApp::GetApp() {
    return app_;
}

}  // namespace chatroom::framework
