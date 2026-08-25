#pragma once

#include <memory>
#include <string>

#include "chat/online_user_service.h"
#include "user/auth_service.h"
#include "websocket/ws_chat_controller.h"
#include "websocket/ws_connection_manager.h"
#include "websocket/ws_message_dispatcher.h"

namespace chatroom::websocket {

// 总调度：负责 WebSocket 连接的开 / 消息 / 关三个时刻。
class WsChatControllerImpl : public WsChatController {
public:
    WsChatControllerImpl(std::shared_ptr<user::AuthService> auth_service,
                         std::shared_ptr<chat::OnlineUserService> online_user_service,
                         std::shared_ptr<WsConnectionManager> connection_manager,
                         std::shared_ptr<WsMessageDispatcher> dispatcher);

    void OnOpen(crow::websocket::connection& connection,
                const std::string& sessionId) override;
    void OnMessage(crow::websocket::connection& connection,
                   const std::string& rawMessage) override;
    void OnClose(crow::websocket::connection& connection) override;

private:
    std::shared_ptr<user::AuthService> auth_service_;
    std::shared_ptr<chat::OnlineUserService> online_user_service_;
    std::shared_ptr<WsConnectionManager> connection_manager_;
    std::shared_ptr<WsMessageDispatcher> dispatcher_;
};

}  // namespace chatroom::websocket
