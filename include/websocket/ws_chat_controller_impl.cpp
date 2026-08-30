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
    // TODO:
    // 1. 用 auth_service_->ValidateSession(sessionId) 拿到 nickname
    // 2. 无效就 connection.close() 并返回
    // 3. 有效就 BindConnection + MarkOnline，并把 nickname 存进 connection.userdata
    auto nickname = auth_service_->ValidateSession (sessionId);
    if(!nickname){
        connection.close ();
        return;
    }
    connection_manager_->BindConnection (*nickname, connection);
    online_user_service_->MarkOnline (*nickname);
    connection.userdata (new std::string(*nickname));
    BroadcastOnlineUsers();
}

void WsChatControllerImpl::OnMessage(crow::websocket::connection& connection,
                                     const std::string& rawMessage) {
    // TODO: 交给 dispatcher_->Dispatch(connection, rawMessage)
    dispatcher_->Dispatch (connection, rawMessage);
}

void WsChatControllerImpl::OnClose(crow::websocket::connection& connection) {
    // TODO:
    // 1. 取出 userdata 里存的 nickname
    // 2. UnbindConnection + MarkOffline
    // 3. delete 掉那个存 nickname 的指针
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
    delete nicknamePointer;
    BroadcastOnlineUsers();
}

}  // namespace chatroom::websocket
