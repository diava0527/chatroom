#include "websocket/ws_message_dispatcher_impl.h"

#include <exception>
#include <optional>
#include <utility>

#include "chat/chat_protocol.h"

namespace {

std::optional<std::string> ReadString(const crow::json::rvalue& object,
                                      const char* key) {
    if (!object.has(key) || object[key].t() != crow::json::type::String) {
        return std::nullopt;
    }
    return std::string(object[key].s());
}

void SendError(crow::websocket::connection& connection,
               const std::string& message) {
    crow::json::wvalue response;
    response["event"] = std::string(chatroom::chat::ChatProtocol::kError);
    response["payload"]["message"] = message;
    connection.send_text(response.dump());
}

void FillMessage(crow::json::wvalue& target,
                 const chatroom::models::Message& message) {
    target["messageId"] = message.messageId;
    target["senderNickname"] = message.senderNickname;
    target["receiverNickname"] = message.receiverNickname;
    target["content"] = message.content;
    target["timestamp"] = message.timestamp;
}

}  // namespace

namespace chatroom::websocket {

WsMessageDispatcherImpl::WsMessageDispatcherImpl(
    std::shared_ptr<chat::LobbyService> lobby_service,
    std::shared_ptr<chat::PrivateChatService> private_chat_service,
    std::shared_ptr<chat::OnlineUserService> online_user_service,
    std::shared_ptr<WsConnectionManager> connection_manager)
    : lobby_service_(std::move(lobby_service)),
      private_chat_service_(std::move(private_chat_service)),
      online_user_service_(std::move(online_user_service)),
      connection_manager_(std::move(connection_manager)) {}

void WsMessageDispatcherImpl::Dispatch(
    crow::websocket::connection& connection,
    const std::string& rawMessage) {
    auto* nicknamePointer = static_cast<std::string*>(connection.userdata());
    if (nicknamePointer == nullptr) {
        return;
    }
    const std::string nickname = *nicknamePointer;

    try {
        const auto message = crow::json::load(rawMessage);
        if (!message || message.t() != crow::json::type::Object) {
            SendError(connection, "invalid JSON message");
            return;
        }

        const auto event = ReadString(message, "event");
        if (!event.has_value()) {
            SendError(connection, "event must be a string");
            return;
        }

        if (*event == chat::ChatProtocol::kLobbyEnter) {
            const std::string enteredAt = lobby_service_->EnterLobby(nickname);
            {
                std::lock_guard<std::mutex> lock(entered_at_mtx_);
                entered_at[nickname] = enteredAt;
            }
            crow::json::wvalue response;
            response["event"] = std::string(chat::ChatProtocol::kLobbyEnterAck);
            response["payload"]["nickname"] = nickname;
            response["payload"]["enteredAt"] = enteredAt;
            connection.send_text(response.dump());
            return;
        }

        if (*event == chat::ChatProtocol::kLobbyHistoryPull) {
            std::string enteredAt;
            {
                std::lock_guard<std::mutex> lock(entered_at_mtx_);
                const auto found = entered_at.find(nickname);
                if (found == entered_at.end()) {
                    SendError(connection, "enter the lobby before pulling history");
                    return;
                }
                enteredAt = found->second;
            }
            const auto messages = lobby_service_->PullVisibleHistory(enteredAt);
            crow::json::wvalue response;
            response["event"] =
                std::string(chat::ChatProtocol::kLobbyHistoryResponse);
            response["payload"]["messages"] = crow::json::wvalue::list{};
            for (std::size_t index = 0; index < messages.size(); ++index) {
                FillMessage(response["payload"]["messages"][index],
                            messages[index]);
            }
            connection.send_text(response.dump());
            return;
        }

        if (!message.has("payload")
            || message["payload"].t() != crow::json::type::Object) {
            SendError(connection, "payload must be an object");
            return;
        }
        const auto& payload = message["payload"];

        if (*event == chat::ChatProtocol::kLobbySend) {
            const auto content = ReadString(payload, "content");
            if (!content.has_value() || content->empty()) {
                SendError(connection, "message content cannot be empty");
                return;
            }
            const auto sent = lobby_service_->SendLobbyMessage(nickname, *content);
            crow::json::wvalue response;
            response["event"] = std::string(chat::ChatProtocol::kLobbyReceive);
            FillMessage(response["payload"], sent);
            connection_manager_->Broadcast(response.dump());
            return;
        }

        if (*event == chat::ChatProtocol::kPrivateHistoryPull) {
            const auto sessionId = ReadString(payload, "privateSessionId");
            if (!sessionId.has_value()
                || !private_chat_service_->IsParticipant(*sessionId, nickname)) {
                SendError(connection, "private session is invalid");
                return;
            }
            const auto messages =
                private_chat_service_->PullPrivateHistory(*sessionId);
            crow::json::wvalue response;
            response["event"] =
                std::string(chat::ChatProtocol::kPrivateHistoryResponse);
            response["payload"]["privateSessionId"] = *sessionId;
            response["payload"]["messages"] = crow::json::wvalue::list{};
            for (std::size_t index = 0; index < messages.size(); ++index) {
                FillMessage(response["payload"]["messages"][index],
                            messages[index]);
            }
            connection.send_text(response.dump());
            return;
        }

        if (*event == chat::ChatProtocol::kPrivateSend) {
            const auto sessionId = ReadString(payload, "privateSessionId");
            const auto content = ReadString(payload, "content");
            if (!sessionId.has_value() || !content.has_value()
                || content->empty()) {
                SendError(connection, "private message payload is invalid");
                return;
            }
            const auto sent = private_chat_service_->SendPrivateMessage(
                *sessionId, nickname, *content);
            if (sent.messageId.empty()) {
                SendError(connection, "private session is invalid");
                return;
            }
            crow::json::wvalue response;
            response["event"] = std::string(chat::ChatProtocol::kPrivateReceive);
            response["payload"]["privateSessionId"] = *sessionId;
            FillMessage(response["payload"]["message"], sent);
            const std::string serialized = response.dump();
            connection.send_text(serialized);
            connection_manager_->SendToUser(sent.receiverNickname, serialized);
            return;
        }

        SendError(connection, "unsupported event");
    } catch (const std::exception&) {
        SendError(connection, "message processing failed");
    }
}

}  // namespace chatroom::websocket
