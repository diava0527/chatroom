#include "storage/mysql/mysql_lobby_store.h"

#include <utility>

#include "storage/mysql/mysql_statement.h"

namespace chatroom::storage::mysql {

MySqlLobbyStore::MySqlLobbyStore(std::shared_ptr<MySqlDatabase> database)
    : database_(std::move(database)) {}

void MySqlLobbyStore::SaveLobbySession(
    const chatroom::models::LobbySession& session) {
    auto connection = database_->OpenConnection();
    MySqlStatement statement(
        connection,
        "INSERT INTO lobby_sessions (nickname, entered_at) VALUES (?, ?)");
    statement.Execute({session.nickname, session.enteredAt});
}

std::optional<std::string> MySqlLobbyStore::FindEnteredAt(
    const std::string& nickname) const {
    auto connection = database_->OpenConnection();
    MySqlStatement statement(
        connection,
        "SELECT MIN(entered_at) FROM lobby_sessions WHERE nickname = ?");
    const auto rows = statement.Query({nickname});
    if (rows.empty() || !rows[0][0].has_value()) {
        return std::nullopt;
    }
    return rows[0][0];
}

void MySqlLobbyStore::AppendLobbyMessage(
    const chatroom::models::Message& message) {
    auto connection = database_->OpenConnection();
    MySqlStatement statement(
        connection,
        "INSERT INTO lobby_messages "
        "(message_id, sender_nickname, receiver_nickname, content, "
        "message_timestamp) VALUES (?, ?, ?, ?, ?)");
    statement.Execute({message.messageId,
                       message.senderNickname,
                       message.receiverNickname,
                       message.content,
                       message.timestamp});
}

std::vector<chatroom::models::Message> MySqlLobbyStore::ListMessagesAfter(
    const std::string& enteredAt) const {
    auto connection = database_->OpenConnection();
    MySqlStatement statement(
        connection,
        "SELECT message_id, sender_nickname, receiver_nickname, content, "
        "message_timestamp FROM lobby_messages "
        "WHERE message_timestamp > ? ORDER BY sequence_id");
    const auto rows = statement.Query({enteredAt});

    std::vector<chatroom::models::Message> messages;
    messages.reserve(rows.size());
    for (const auto& row : rows) {
        messages.push_back({row[0].value_or(""),
                            row[1].value_or(""),
                            row[2].value_or(""),
                            row[3].value_or(""),
                            row[4].value_or("")});
    }
    return messages;
}

}  // namespace chatroom::storage::mysql
