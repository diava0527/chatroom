#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "chat/lobby_service.h"
#include "chat/online_user_service.h"
#include "chat/private_chat_service.h"
#include "websocket/ws_connection_manager.h"
#include "websocket/ws_message_dispatcher.h"

namespace chatroom::websocket {

	// 解析消息的 event 字段，分发到对应流程。
	class WsMessageDispatcherImpl : public WsMessageDispatcher {
	public:
		WsMessageDispatcherImpl (std::shared_ptr<chat::LobbyService> lobby_service,
			std::shared_ptr<chat::PrivateChatService> private_chat_service,
			std::shared_ptr<chat::OnlineUserService> online_user_service,
			std::shared_ptr<WsConnectionManager> connection_manager);

		void Dispatch (crow::websocket::connection& connection,
			const std::string& rawMessage) override;

	private:
		std::shared_ptr<chat::LobbyService> lobby_service_;
		std::shared_ptr<chat::PrivateChatService> private_chat_service_;
		std::shared_ptr<chat::OnlineUserService> online_user_service_;
		std::shared_ptr<WsConnectionManager> connection_manager_;//用于广播和私聊，在manager里定义了
		std::unordered_map<std::string, std::string> entered_at;//记录进入大厅的时间，nickname->enteredAt
		std::mutex entered_at_mtx_;//加锁，防止entered_at同时读写
	};

}  // namespace chatroom::websocket
