#define CROW_USE_BOOST 1
#define NOMINMAX

#include "crow_all.h"

#include <cstdlib>
#include <chrono>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

#include "chat/chat_controller_impl.h"
#include "chat/lobby_service_impl.h"
#include "chat/online_user_service_impl.h"
#include "chat/private_chat_service_impl.h"
#include "common/constants.h"
#include "framework/crow_router_registry.h"
#include "framework/crow_server_app.h"
#include "framework/json_response_builder.h"
#include "user/auth_controller_impl.h"
#include "user/auth_service_impl.h"
#include "websocket/ws_chat_controller_impl.h"
#include "websocket/ws_connection_manager_impl.h"
#include "websocket/ws_message_dispatcher_impl.h"

#if defined(CHATROOM_ENABLE_MYSQL)
#include "storage/mysql/mysql_database.h"
#include "storage/mysql/mysql_lobby_store.h"
#include "storage/mysql/mysql_private_chat_store.h"
#include "storage/mysql/mysql_schema.h"
#include "storage/mysql/mysql_user_store.h"
#endif

namespace {

constexpr std::uint16_t kServerPort = 8080;
constexpr const char* kBindAddress = "127.0.0.1";
constexpr const char* kDefaultFrontendOrigin = "http://localhost:8080";
constexpr const char* kDefaultFrontendDir = "frontend";
constexpr const char* kSessionHeader = "X-Session-Id";

std::string ReadEnvironmentOrDefault(const char* name,
                                     const char* defaultValue) {
    const char* value = std::getenv(name);
    return value == nullptr || *value == '\0' ? defaultValue : value;
}

crow::response BuildErrorResponse(int httpStatus,
                                  int code,
                                  const std::string& message) {
    crow::json::wvalue body;
    body["code"] = code;
    body["message"] = message;
    body["data"] = nullptr;
    crow::response response(httpStatus, body);
    response.set_header("Content-Type", "application/json; charset=utf-8");
    return response;
}

// 把浏览器请求的静态资源路径映射到前端目录下的真实文件。
// 页面和接口由同一个 Crow 进程、同一个端口提供，因此天然同源，无需再开静态服务器。
crow::response ServeFrontendFile(const std::filesystem::path& frontendDir,
                                 const std::string& relativePath) {
    // 拒绝空路径与上跳目录，避免越权读取前端目录之外的文件
    if (relativePath.empty()
        || relativePath.find("..") != std::string::npos) {
        return BuildErrorResponse(404, 1404, "not found");
    }

    crow::response response;
    // set_static_file_info 会按扩展名推断 Content-Type，文件不存在时自动返回 404
    response.set_static_file_info((frontendDir / relativePath).string());
    return response;
}

crow::response RunHttpAction(
    const std::function<crow::response()>& action) {
    try {
        return action();
    } catch (const std::exception& error) {
        std::cerr << "HTTP request failed: " << error.what() << '\n';
        return BuildErrorResponse(500, 1500, "internal server error");
    }
}

void BroadcastOnlineUsers(
    const std::shared_ptr<chatroom::chat::OnlineUserService>& onlineUserService,
    const std::shared_ptr<chatroom::websocket::WsConnectionManager>&
        connectionManager) {
    crow::json::wvalue response;
    response["event"] = "online.users.changed";
    response["payload"]["users"] = crow::json::wvalue::list{};
    const auto users = onlineUserService->ListOnlineUsers();
    for (std::size_t index = 0; index < users.size(); ++index) {
        response["payload"]["users"][index] = users[index];
    }
    connectionManager->Broadcast(response.dump());
}

}  // namespace

int main() {
#if !defined(CHATROOM_ENABLE_MYSQL)
    std::cerr << "The chatroom server requires CHATROOM_ENABLE_MYSQL=ON\n";
    return 1;
#else
    try {
        auto mysqlConfig =
            chatroom::storage::mysql::MySqlConfig::FromEnvironment();
        if (mysqlConfig.database.empty()) {
            throw std::invalid_argument("DB_NAME is required");
        }

        auto database =
            std::make_shared<chatroom::storage::mysql::MySqlDatabase>(
                std::move(mysqlConfig));
        chatroom::storage::mysql::InitializeChatroomSchema(*database);

        auto userStore =
            std::make_shared<chatroom::storage::mysql::MySqlUserStore>(database);
        auto lobbyStore =
            std::make_shared<chatroom::storage::mysql::MySqlLobbyStore>(database);
        auto privateChatStore =
            std::make_shared<chatroom::storage::mysql::MySqlPrivateChatStore>(
                database);

        auto lobbyService =
            std::make_shared<chatroom::chat::LobbyServiceImpl>(lobbyStore);
        auto privateChatService =
            std::make_shared<chatroom::chat::PrivateChatServiceImpl>(
                privateChatStore);
        auto authService =
            std::make_shared<chatroom::user::AuthServiceImpl>(
                userStore, [privateChatService](const std::string& nickname) {
                    privateChatService->ClearSessionsByUser(nickname);
                });
        auto onlineUserService =
            std::make_shared<chatroom::chat::OnlineUserServiceImpl>();
        auto responseBuilder =
            std::make_shared<chatroom::framework::JsonResponseBuilder>();

        auto authController =
            std::make_shared<chatroom::user::AuthControllerImpl>(authService);
        auto chatController =
            std::make_shared<chatroom::chat::ChatControllerImpl>(
                authService,
                privateChatService,
                onlineUserService,
                responseBuilder);

        auto connectionManager =
            std::make_shared<chatroom::websocket::WsConnectionManagerImpl>();
        auto dispatcher =
            std::make_shared<chatroom::websocket::WsMessageDispatcherImpl>(
                lobbyService,
                privateChatService,
                onlineUserService,
                connectionManager);
        auto wsController =
            std::make_shared<chatroom::websocket::WsChatControllerImpl>(
                authService,
                onlineUserService,
                connectionManager,
                dispatcher);

        chatroom::framework::CrowRouterRegistry routerRegistry;
        chatroom::framework::CrowServerApp serverApp(
            routerRegistry,
            kServerPort,
            kBindAddress,
            ReadEnvironmentOrDefault(
                "FRONTEND_ORIGIN", kDefaultFrontendOrigin));

        routerRegistry.AddRegistrar(
            [=](chatroom::framework::ChatroomApp& app) {
                app.tick(std::chrono::seconds(15), [authService] {
                    try {
                        authService->ExpireDisconnectedSessions();
                    } catch (const std::exception& error) {
                        std::cerr << "Session cleanup failed: " << error.what() << '\n';
                    }
                });

                CROW_ROUTE(app, "/api/v1/health")([database] {
                    try {
                        auto connection = database->OpenConnection();
                        connection.Ping();
                        crow::json::wvalue body;
                        body["code"] = 0;
                        body["message"] = "server started";
                        body["data"]["database"] = "connected";
                        crow::response response(200, body);
                        response.set_header(
                            "Content-Type", "application/json; charset=utf-8");
                        return response;
                    } catch (const std::exception& error) {
                        std::cerr << "Health check failed: " << error.what() << '\n';
                        return BuildErrorResponse(
                            503, 1500, "database unavailable");
                    }
                });

                CROW_ROUTE(app, "/api/v1/auth/register")
                    .methods(crow::HTTPMethod::Post)(
                        [authController](const crow::request& request) {
                            return RunHttpAction([&] {
                                return authController->Register(request);
                            });
                        });

                CROW_ROUTE(app, "/api/v1/auth/login")
                    .methods(crow::HTTPMethod::Post)(
                        [authController](const crow::request& request) {
                            return RunHttpAction([&] {
                                return authController->Login(request);
                            });
                        });

                CROW_ROUTE(app, "/api/v1/auth/logout")
                    .methods(crow::HTTPMethod::Post)(
                        [=](const crow::request& request) {
                            return RunHttpAction([&] {
                                const std::string sessionId =
                                    request.get_header_value(kSessionHeader);
                                const auto nickname =
                                    authService->ValidateSession(sessionId);
                                auto response = authController->Logout(request);
                                if (nickname.has_value() && response.code == 200) {
                                    onlineUserService->MarkOffline(*nickname);
                                    BroadcastOnlineUsers(
                                        onlineUserService, connectionManager);
                                }
                                return response;
                            });
                        });

                CROW_ROUTE(app, "/api/v1/users/online")
                    .methods(crow::HTTPMethod::Get)(
                        [chatController](const crow::request& request) {
                            return RunHttpAction([&] {
                                return chatController->ListOnlineUsers(request);
                            });
                        });

                CROW_ROUTE(app, "/api/v1/private-chat/session")
                    .methods(crow::HTTPMethod::Post)(
                        [chatController](const crow::request& request) {
                            return RunHttpAction([&] {
                                return chatController->CreatePrivateSession(
                                    request);
                            });
                        });

                CROW_WEBSOCKET_ROUTE(app, "/ws/chat")
                    .onaccept(
                        [authService](const crow::request& request,
                                      std::optional<crow::response>& response,
                                      void** userData) {
                            const char* rawSessionId =
                                request.url_params.get("sessionId");
                            const std::string sessionId =
                                rawSessionId == nullptr ? "" : rawSessionId;
                            if (!authService->ValidateSession(sessionId)
                                     .has_value()) {
                                response = BuildErrorResponse(
                                    401, 1003, "sessionId invalid");
                                return;
                            }
                            *userData = new std::string(sessionId);
                        })
                    .onopen(
                        [wsController](crow::websocket::connection& connection) {
                            auto* sessionIdPointer = static_cast<std::string*>(
                                connection.userdata());
                            if (sessionIdPointer == nullptr) {
                                connection.close();
                                return;
                            }
                            const std::string sessionId = *sessionIdPointer;
                            delete sessionIdPointer;
                            connection.userdata(nullptr);
                            wsController->OnOpen(connection, sessionId);
                        })
                    .onmessage(
                        [wsController](crow::websocket::connection& connection,
                                       const std::string& message,
                                       bool isBinary) {
                            if (isBinary) {
                                return;
                            }
                            wsController->OnMessage(connection, message);
                        })
                    .onclose(
                        [wsController](crow::websocket::connection& connection,
                                       const std::string&,
                                       std::uint16_t) {
                            wsController->OnClose(connection);
                        });

                // 前端页面与静态资源由后端一起提供，页面和接口同源。
                // 这两条路由必须注册在最后：Crow 在多个规则都能匹配时取注册序号更小的那条，
                // 所以 /api/v1/* 与 /ws/chat 仍然优先，不会被静态路由抢走。
                const std::filesystem::path frontendDir(
                    ReadEnvironmentOrDefault("FRONTEND_DIR",
                                             kDefaultFrontendDir));
                CROW_ROUTE(app, "/")([frontendDir] {
                    return ServeFrontendFile(frontendDir, "login.html");
                });
                CROW_ROUTE(app, "/<path>")([frontendDir](std::string path) {
                    return ServeFrontendFile(frontendDir, path);
                });
            });

        std::cout << "Chatroom backend starting at http://127.0.0.1:8080\n";
        std::cout << "Serving frontend from: "
                  << ReadEnvironmentOrDefault("FRONTEND_DIR",
                                              kDefaultFrontendDir)
                  << '\n';
        serverApp.Run();
        return 0;
    } catch (const chatroom::storage::mysql::MySqlError& error) {
        std::cerr << "MySQL startup failed (error " << error.ErrorCode()
                  << "): " << error.what() << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Application startup failed: " << error.what() << '\n';
    }
    return 1;
#endif
}
