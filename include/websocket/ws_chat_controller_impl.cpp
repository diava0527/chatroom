#include "websocket/ws_chat_controller_impl.h"

#include <algorithm>
#include <utility>

#include "chat/chat_protocol.h"

namespace chatroom::websocket {

WsChatControllerImpl::WsChatControllerImpl(
    std::shared_ptr<user::AuthService> auth_service,
    std::shared_ptr<chat::OnlineUserService> online_user_service,
    std::shared_ptr<WsConnectionManager> connection_manager,
    std::shared_ptr<WsMessageDispatcher> dispatcher)
    : auth_service_(std::move(auth_service)),
      online_user_service_(std::move(online_user_service)),
      connection_manager_(std::move(connection_manager)),
      dispatcher_(std::move(dispatcher)) {}

void WsChatControllerImpl::BroadcastOnlineUsers() {
    crow::json::wvalue response;
    response["event"] = std::string(chat::ChatProtocol::kOnlineUsersChanged);
    response["payload"]["users"] = crow::json::wvalue::list{};
    const auto users = online_user_service_->ListOnlineUsers();
    for (std::size_t index = 0; index < users.size(); ++index) {
        response["payload"]["users"][index] = users[index];
    }
    connection_manager_->Broadcast(response.dump());
}

void WsChatControllerImpl::OnOpen(crow::websocket::connection& connection,
                                  const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(lifecycleMutex_);
    auto nickname = auth_service_->AttachConnection(sessionId, &connection);
    if(!nickname){
        connection.close ();
        return;
    }
    connectionSessions_[&connection] = sessionId;
    connection_manager_->BindConnection (*nickname, connection);
    online_user_service_->MarkOnline (*nickname);
    connection.userdata (new std::string(*nickname));
    BroadcastOnlineUsers();
}

void WsChatControllerImpl::OnMessage(crow::websocket::connection& connection,
                                     const std::string& rawMessage) {
    std::lock_guard<std::mutex> lock(lifecycleMutex_);
    const auto session = connectionSessions_.find(&connection);
    if (session == connectionSessions_.end()
        || !auth_service_->ValidateConnection(session->second, &connection)) {
        connection.close("session expired");
        return;
    }
    dispatcher_->Dispatch (connection, rawMessage);
}

void WsChatControllerImpl::OnClose(crow::websocket::connection& connection) {
    std::lock_guard<std::mutex> lock(lifecycleMutex_);
    auto* nicknamePointer = static_cast<std::string*>(connection.userdata());
    if (nicknamePointer == nullptr) {
        return;
    }
    const std::string nickname = *nicknamePointer;
    connection.userdata(nullptr);
    connection_manager_->UnbindConnection (connection);
    const auto connectedUsers = connection_manager_->ListConnectedUsers();
    if (std::find(connectedUsers.begin(), connectedUsers.end(), nickname)
        == connectedUsers.end()) {
        online_user_service_->MarkOffline(nickname);
    }
    const auto session = connectionSessions_.find(&connection);
    if (session != connectionSessions_.end()) {
        auth_service_->DetachConnection(session->second, &connection);
        connectionSessions_.erase(session);
    }
    delete nicknamePointer;
    BroadcastOnlineUsers();
}

}  // namespace chatroom::websocket
