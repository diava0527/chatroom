// 仅用于集成测试：真实 HTTP/WebSocket 控制器，隔离的内存账号，不接触用户数据库。
#include <cstdlib>
#include <memory>
#include <string>
#include "crow_all.h"
#include "user/auth_controller_impl.h"
#include "user/auth_service_impl.h"
#include "storage/user_memory_store_impl.h"
#include "chat/online_user_service_impl.h"
#include "websocket/ws_chat_controller_impl.h"
#include "websocket/ws_connection_manager_impl.h"

class EchoDispatcher final : public chatroom::websocket::WsMessageDispatcher {
public:
    void Dispatch(crow::websocket::connection& connection, const std::string& message) override {
        connection.send_text(message);
    }
};

int main(int argc, char** argv) {
    if (argc != 2) return 1;
    auto auth = std::make_shared<chatroom::user::AuthServiceImpl>(
        std::make_shared<chatroom::storage::UserMemoryStoreImpl>());
    auto http = std::make_shared<chatroom::user::AuthControllerImpl>(auth);
    auto ws = std::make_shared<chatroom::websocket::WsChatControllerImpl>(auth,
        std::make_shared<chatroom::chat::OnlineUserServiceImpl>(),
        std::make_shared<chatroom::websocket::WsConnectionManagerImpl>(),
        std::make_shared<EchoDispatcher>());
    crow::SimpleApp app;
    app.loglevel(crow::LogLevel::Warning);
    CROW_ROUTE(app, "/health")([] { return "ready"; });
    CROW_ROUTE(app, "/api/v1/auth/register").methods(crow::HTTPMethod::Post)(
        [http](const crow::request& request) { return http->Register(request); });
    CROW_ROUTE(app, "/api/v1/auth/login").methods(crow::HTTPMethod::Post)(
        [http](const crow::request& request) { return http->Login(request); });
    CROW_ROUTE(app, "/api/v1/auth/logout").methods(crow::HTTPMethod::Post)(
        [http](const crow::request& request) { return http->Logout(request); });
    CROW_WEBSOCKET_ROUTE(app, "/ws/chat")
        .onaccept([auth](const crow::request& request, std::optional<crow::response>& response, void** data) {
            const auto raw = request.url_params.get("sessionId");
            const std::string session = raw ? raw : "";
            if (!auth->ValidateSession(session)) { response = crow::response(401); return; }
            *data = new std::string(session);
        })
        .onopen([ws](crow::websocket::connection& connection) {
            auto* raw = static_cast<std::string*>(connection.userdata());
            const auto session = *raw;
            delete raw;
            connection.userdata(nullptr);
            ws->OnOpen(connection, session);
        })
        .onmessage([ws](crow::websocket::connection& connection, const std::string& message, bool) {
            ws->OnMessage(connection, message);
        })
        .onclose([ws](crow::websocket::connection& connection, const std::string&, std::uint16_t) {
            ws->OnClose(connection);
        });
    app.bindaddr("127.0.0.1").port(static_cast<std::uint16_t>(std::stoi(argv[1]))).concurrency(2).run();
}
