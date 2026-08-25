#define CROW_USE_BOOST 1

#include "crow_all.h"

#include <memory>
#include <optional>

#include "common/constants.h"
#include "framework/crow_router_registry.h"
#include "framework/crow_server_app.h"
#include "framework/json_response_builder.h"

int main() {
    auto responseBuilder = std::make_shared<chatroom::framework::JsonResponseBuilder>();

    chatroom::framework::CrowRouterRegistry routerRegistry;
    chatroom::framework::CrowServerApp serverApp(routerRegistry);

    routerRegistry.AddRegistrar([responseBuilder](crow::SimpleApp& app) {
        CROW_ROUTE(app, "/")([] {
            return "chatroom backend is running";
        });

        CROW_ROUTE(app, "/api/v1/health")([responseBuilder] {
            return responseBuilder->BuildHttpJson(
                chatroom::common::kSuccessCode,
                "server started",
                std::nullopt);
        });

        // 后续业务模块完成后，可继续通过路由注册中心分批挂载 HTTP 与 WebSocket 路由。
    });

    serverApp.Run();
    return 0;
}
