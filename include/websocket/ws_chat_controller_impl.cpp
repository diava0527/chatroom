#include "websocket/ws_chat_controller_impl.h"

#include <utility>

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

void WsChatControllerImpl::OnOpen(crow::websocket::connection& connection,
                                  const std::string& sessionId) {
    // TODO:
    // 1. 用 auth_service_->ValidateSession(sessionId) 拿到 nickname
    // 2. 无效就 connection.close() 并返回
    // 3. 有效就 BindConnection + MarkOnline，并把 nickname 存进 connection.userdata
}

void WsChatControllerImpl::OnMessage(crow::websocket::connection& connection,
                                     const std::string& rawMessage) {
    // TODO: 交给 dispatcher_->Dispatch(connection, rawMessage)
}

void WsChatControllerImpl::OnClose(crow::websocket::connection& connection) {
    // TODO:
    // 1. 取出 userdata 里存的 nickname
    // 2. UnbindConnection + MarkOffline
    // 3. delete 掉那个存 nickname 的指针
}

}  // namespace chatroom::websocket
