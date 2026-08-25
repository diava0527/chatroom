#include "websocket/ws_message_dispatcher_impl.h"
#include"chat/chat_protocol.h"
#include <utility>

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

void WsMessageDispatcherImpl::Dispatch (crow::websocket::connection& connection,
    const std::string& rawMessage) {
    // 1. 用 crow::json::load 解析 rawMessage
    // 2. 读出 event 字段
    // 3. 根据 event 分发：
    //    - lobby.enter          -> 进大厅
    //    - lobby.message.send   -> 发大厅消息 + 广播
    //    - lobby.history.pull   -> 拉大厅历史
    //    - private.message.send -> 发私聊消息 + 单播
    //    - private.history.pull -> 拉私聊历史
    auto msg = crow::json::load (rawMessage);
    if (!msg)//检查是否返回错误对象
        return;
    std::string event = msg["event"].s ();
    auto temp = static_cast<std::string*>(connection.userdata ());//userdate()获取由我挂载于connection的数据（挂载了一个昵称）
    std::string nickname = *temp;

    //分发
    if (event == chat::ChatProtocol::kLobbyEnter) {
        std::string enteredAt = lobby_service_->EnterLobby (nickname);//获取进入大厅的时间
        {
            std::lock_guard<std::mutex> lock (entered_at_mtx_);
            entered_at[nickname] = enteredAt;//记录昵称对应的进入大厅时间，以便后续获取历史记录
        }
        crow::json::wvalue response;
        response["event"] = static_cast<std::string>(chat::ChatProtocol::kLobbyEnterAck);
        response["payload"]["nickname"] = nickname;
        response["payload"]["enteredAt"] = enteredAt;
        connection.send_text (response.dump ());//转为json形式，send_text只能返回给connection自己（响应）
    }
    else if (event == chat::ChatProtocol::kLobbySend) {
        auto message = lobby_service_->SendLobbyMessage (nickname, msg["payload"]["content"].s ());
        crow::json::wvalue response;
        response["event"] = static_cast<std::string>(chat::ChatProtocol::kLobbyReceive);
        response["payload"]["messageId"] = message.messageId;
        response["payload"]["senderNickname"] = message.senderNickname;
        response["payload"]["receiverNickname"] = message.receiverNickname;
        response["payload"]["content"] = message.content;
        response["payload"]["timestamp"] = message.timestamp;
        connection_manager_->Broadcast (response.dump ());//广播给全体
    }
    else if (event == chat::ChatProtocol::kLobbyHistoryPull) {
        std::string enteredAt;
        {
            std::lock_guard<std::mutex> lock (entered_at_mtx_);
            enteredAt = entered_at[nickname];
        }
        auto messages = lobby_service_->PullVisibleHistory (enteredAt);
        crow::json::wvalue response;
        response["event"] = static_cast<std::string>(chat::ChatProtocol::kLobbyHistoryResponse);
        for (size_t i = 0; i < messages.size (); ++i) {
            response["payload"]["messages"][i]["messageId"] = messages[i].messageId;
            response["payload"]["messages"][i]["senderNickname"] = messages[i].senderNickname;
            response["payload"]["messages"][i]["receiverNickname"] = messages[i].receiverNickname;
            response["payload"]["messages"][i]["content"] = messages[i].content;
            response["payload"]["messages"][i]["timestamp"] = messages[i].timestamp;
        }
        connection.send_text (response.dump ());
    }
    else if (event == chat::ChatProtocol::kPrivateSend) {
        std::string sessionID = msg["payload"]["privateSessionId"].s ();
        std::string content = msg["payload"]["content"].s ();
        auto m = private_chat_service_->SendPrivateMessage (sessionID, nickname, content);
        crow::json::wvalue response;
        response["event"] = static_cast<std::string>(chat::ChatProtocol::kPrivateReceive);
        response["payload"]["privateSessionId"] = sessionID;
        response["payload"]["message"]["messageId"] = m.messageId;
        response["payload"]["message"]["senderNickname"] = m.senderNickname;
        response["payload"]["message"]["receiverNickname"] = m.receiverNickname;
        response["payload"]["message"]["content"] = m.content;
        response["payload"]["message"]["timestamp"] = m.timestamp;
        connection_manager_->SendToUser (m.receiverNickname, response.dump ());//私发给对应用户
    }
    else if (event == chat::ChatProtocol::kPrivateHistoryPull){
        std::string sessionID = msg["payload"]["privateSessionId"].s ();
        auto messages = private_chat_service_->PullPrivateHistory (sessionID);//获取历史记录数组
        crow::json::wvalue response;
        response["event"] = static_cast<std::string>(chat::ChatProtocol::kPrivateHistoryResponse);
        response["payload"]["privateSessionId"] = sessionID;
        for (size_t i = 0; i < messages.size (); i++) {
            response["payload"]["messages"][i]["messageId"] = messages[i].messageId;
            response["payload"]["messages"][i]["senderNickname"] = messages[i].senderNickname;
            response["payload"]["messages"][i]["receiverNickname"] = messages[i].receiverNickname;
            response["payload"]["messages"][i]["content"] = messages[i].content;
            response["payload"]["messages"][i]["timestamp"] = messages[i].timestamp;
        }
        connection.send_text (response.dump ());
    }
}

}  // namespace chatroom::websocket
