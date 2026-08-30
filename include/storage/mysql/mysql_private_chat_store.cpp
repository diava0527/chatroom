#include "storage/mysql/mysql_private_chat_store.h"

#include <utility>

#include "common/random_id.h"
#include "storage/mysql/mysql_statement.h"

namespace chatroom::storage::mysql {

MySqlPrivateChatStore::MySqlPrivateChatStore(
    std::shared_ptr<MySqlDatabase> database)
    : database_(std::move(database)) {}

chatroom::models::PrivateChatSession MySqlPrivateChatStore::CreateSession(
    const std::string& senderNickname,
    const std::string& receiverNickname) {
    for (int attempt = 0; attempt < 5; ++attempt) {
        const std::string sessionId = chatroom::common::GenerateRandomHexId();
        try {
            auto connection = database_->OpenConnection();
            MySqlStatement statement(
                connection,
                "INSERT INTO private_chat_sessions "
                "(private_session_id, sender_nickname, receiver_nickname) "
                "VALUES (?, ?, ?)");
            statement.Execute({sessionId, senderNickname, receiverNickname});
            return {sessionId, senderNickname, receiverNickname, {}};
        } catch (const MySqlError& error) {
            if (error.ErrorCode() != 1062) {
                throw;
            }
        }
    }
    throw MySqlError(1062, "failed to generate a unique private session id");
}

std::optional<chatroom::models::PrivateChatSession>
MySqlPrivateChatStore::FindSession(
    const std::string& privateSessionId) const {
    auto connection = database_->OpenConnection();
    MySqlStatement statement(
        connection,
        "SELECT private_session_id, sender_nickname, receiver_nickname "
        "FROM private_chat_sessions WHERE private_session_id = ? LIMIT 1");
    const auto rows = statement.Query({privateSessionId});
    if (rows.empty()) {
        return std::nullopt;
    }
    return chatroom::models::PrivateChatSession{
        rows[0][0].value_or(""),
        rows[0][1].value_or(""),
        rows[0][2].value_or(""),
        {}};
}

bool MySqlPrivateChatStore::AppendMessage(
    const std::string& privateSessionId,
    const chatroom::models::Message& message) {
    try {
        auto connection = database_->OpenConnection();
        MySqlStatement statement(
            connection,
            "INSERT INTO private_messages "
            "(message_id, private_session_id, sender_nickname, "
            "receiver_nickname, content, message_timestamp) "
            "VALUES (?, ?, ?, ?, ?, ?)");
        return statement.Execute({message.messageId,
                                  privateSessionId,
                                  message.senderNickname,
                                  message.receiverNickname,
                                  message.content,
                                  message.timestamp}) == 1;
    } catch (const MySqlError& error) {
        if (error.ErrorCode() == 1452) {
            return false;
        }
        throw;
    }
}

std::vector<chatroom::models::Message>
MySqlPrivateChatStore::ListMessages(
    const std::string& privateSessionId) const {
    auto connection = database_->OpenConnection();
    MySqlStatement statement(
        connection,
        "SELECT message_id, sender_nickname, receiver_nickname, content, "
        "message_timestamp FROM private_messages "
        "WHERE private_session_id = ? ORDER BY sequence_id");
    const auto rows = statement.Query({privateSessionId});

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

void MySqlPrivateChatStore::RemoveSessionsByUser(
    const std::string& nickname) {
    auto connection = database_->OpenConnection();
    MySqlStatement statement(
        connection,
        "DELETE FROM private_chat_sessions "
        "WHERE sender_nickname = ? OR receiver_nickname = ?");
    statement.Execute({nickname, nickname});
}

}  // namespace chatroom::storage::mysql
