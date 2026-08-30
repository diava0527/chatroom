#include "storage/mysql/mysql_schema.h"

#include <array>
#include <string_view>

#include "storage/mysql/mysql_statement.h"

namespace chatroom::storage::mysql {

void InitializeChatroomSchema(const MySqlDatabase& database) {
    static constexpr std::array<std::string_view, 5> kStatements = {
        R"SQL(CREATE TABLE IF NOT EXISTS users (
            nickname VARCHAR(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_as_cs NOT NULL,
            password VARCHAR(255) NOT NULL,
            created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
            PRIMARY KEY (nickname)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_as_cs)SQL",
        R"SQL(CREATE TABLE IF NOT EXISTS lobby_sessions (
            id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
            nickname VARCHAR(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_as_cs NOT NULL,
            entered_at VARCHAR(32) NOT NULL,
            PRIMARY KEY (id),
            KEY idx_lobby_sessions_nickname (nickname),
            CONSTRAINT fk_lobby_sessions_user FOREIGN KEY (nickname)
                REFERENCES users(nickname)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_as_cs)SQL",
        R"SQL(CREATE TABLE IF NOT EXISTS lobby_messages (
            sequence_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
            message_id VARCHAR(64) NOT NULL,
            sender_nickname VARCHAR(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_as_cs NOT NULL,
            receiver_nickname VARCHAR(64) NOT NULL,
            content TEXT NOT NULL,
            message_timestamp VARCHAR(32) NOT NULL,
            PRIMARY KEY (sequence_id),
            UNIQUE KEY uq_lobby_messages_id (message_id),
            KEY idx_lobby_messages_time (message_timestamp, sequence_id),
            CONSTRAINT fk_lobby_messages_sender FOREIGN KEY (sender_nickname)
                REFERENCES users(nickname)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_as_cs)SQL",
        R"SQL(CREATE TABLE IF NOT EXISTS private_chat_sessions (
            private_session_id VARCHAR(64) NOT NULL,
            sender_nickname VARCHAR(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_as_cs NOT NULL,
            receiver_nickname VARCHAR(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_as_cs NOT NULL,
            created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
            PRIMARY KEY (private_session_id),
            KEY idx_private_sessions_sender (sender_nickname),
            KEY idx_private_sessions_receiver (receiver_nickname),
            CONSTRAINT fk_private_sessions_sender FOREIGN KEY (sender_nickname)
                REFERENCES users(nickname),
            CONSTRAINT fk_private_sessions_receiver FOREIGN KEY (receiver_nickname)
                REFERENCES users(nickname)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_as_cs)SQL",
        R"SQL(CREATE TABLE IF NOT EXISTS private_messages (
            sequence_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
            message_id VARCHAR(64) NOT NULL,
            private_session_id VARCHAR(64) NOT NULL,
            sender_nickname VARCHAR(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_as_cs NOT NULL,
            receiver_nickname VARCHAR(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_as_cs NOT NULL,
            content TEXT NOT NULL,
            message_timestamp VARCHAR(32) NOT NULL,
            PRIMARY KEY (sequence_id),
            UNIQUE KEY uq_private_messages_id (message_id),
            KEY idx_private_messages_session (private_session_id, sequence_id),
            CONSTRAINT fk_private_messages_session FOREIGN KEY (private_session_id)
                REFERENCES private_chat_sessions(private_session_id) ON DELETE CASCADE,
            CONSTRAINT fk_private_messages_sender FOREIGN KEY (sender_nickname)
                REFERENCES users(nickname),
            CONSTRAINT fk_private_messages_receiver FOREIGN KEY (receiver_nickname)
                REFERENCES users(nickname)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_as_cs)SQL"
    };

    auto connection = database.OpenConnection();
    for (const auto sql : kStatements) {
        MySqlStatement statement(connection, std::string(sql));
        statement.Execute();
    }
}

}  // namespace chatroom::storage::mysql
