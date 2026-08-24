#define CROW_USE_BOOST 1

#include "crow_all.h"

#include "chat/chat_controller.h"
#include "chat/chat_protocol.h"
#include "chat/lobby_service.h"
#include "chat/online_user_service.h"
#include "chat/private_chat_service.h"
#include "common/constants.h"
#include "common/types.h"
#include "framework/crow_router_registry.h"
#include "framework/crow_server_app.h"
#include "framework/json_response_builder.h"
#include "framework/response_builder.h"
#include "framework/router_registry.h"
#include "framework/server_app.h"
#include "models/lobby_session.h"
#include "models/message.h"
#include "models/private_chat_session.h"
#include "models/user.h"
#include "storage/lobby_memory_store.h"
#include "storage/private_chat_memory_store.h"
#include "storage/session_memory_store.h"
#include "storage/user_memory_store.h"
#include "user/auth_controller.h"
#include "user/auth_service.h"
#include "user/user_service.h"
#include "websocket/ws_chat_controller.h"
#include "websocket/ws_connection_manager.h"
#include "websocket/ws_message_dispatcher.h"

int main() {
    chatroom::framework::CrowRouterRegistry routerRegistry;
    chatroom::framework::JsonResponseBuilder responseBuilder;
    chatroom::framework::CrowServerApp serverApp(routerRegistry);

    routerRegistry.AddRegistrar([&responseBuilder](crow::SimpleApp& app) {
        CROW_ROUTE(app, "/")
        ([] {
            return "chatroom backend is running";
        });

        CROW_ROUTE(app, "/api/v1/health")
        ([&responseBuilder] {
            return responseBuilder.BuildHttpJson(
                chatroom::common::kSuccessCode,
                "server started",
                std::nullopt);
        });
    });

    serverApp.Run();
    return 0;
}
