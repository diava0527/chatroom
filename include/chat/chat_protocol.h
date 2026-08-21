#pragma once

#include <string_view>

namespace chatroom::chat {

struct ChatProtocol {
    static constexpr std::string_view kLobbyEnter = "lobby.enter";
    static constexpr std::string_view kLobbyEnterAck = "lobby.enter.ack";
    static constexpr std::string_view kLobbySend = "lobby.message.send";
    static constexpr std::string_view kLobbyReceive = "lobby.message.receive";
    static constexpr std::string_view kLobbyHistoryPull = "lobby.history.pull";
    static constexpr std::string_view kLobbyHistoryResponse = "lobby.history.response";
    static constexpr std::string_view kPrivateSend = "private.message.send";
    static constexpr std::string_view kPrivateReceive = "private.message.receive";
    static constexpr std::string_view kPrivateHistoryPull = "private.history.pull";
    static constexpr std::string_view kPrivateHistoryResponse = "private.history.response";
    static constexpr std::string_view kOnlineUsersChanged = "online.users.changed";
};

}  // namespace chatroom::chat
