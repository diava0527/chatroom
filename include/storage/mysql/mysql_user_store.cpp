#include "storage/mysql/mysql_user_store.h"

#include <utility>

#include "storage/mysql/mysql_statement.h"

namespace chatroom::storage::mysql {

MySqlUserStore::MySqlUserStore(std::shared_ptr<MySqlDatabase> database)
    : database_(std::move(database)) {}

bool MySqlUserStore::SaveUser(const chatroom::models::User& user) {
    try {
        auto connection = database_->OpenConnection();
        MySqlStatement statement(
            connection,
            "INSERT INTO users (nickname, password) VALUES (?, ?)");
        return statement.Execute({user.nickname, user.password}) == 1;
    } catch (const MySqlError& error) {
        if (error.ErrorCode() == 1062) {
            return false;
        }
        throw;
    }
}

std::optional<chatroom::models::User> MySqlUserStore::FindUserByNickname(
    const std::string& nickname) const {
    auto connection = database_->OpenConnection();
    MySqlStatement statement(
        connection,
        "SELECT nickname, password FROM users WHERE nickname = ? LIMIT 1");
    const auto rows = statement.Query({nickname});
    if (rows.empty()) {
        return std::nullopt;
    }
    return chatroom::models::User{rows[0][0].value_or(""),
                                  rows[0][1].value_or("")};
}

std::vector<chatroom::models::User> MySqlUserStore::ListUsers() const {
    auto connection = database_->OpenConnection();
    MySqlStatement statement(
        connection,
        "SELECT nickname, password FROM users ORDER BY nickname");
    const auto rows = statement.Query();
    std::vector<chatroom::models::User> users;
    users.reserve(rows.size());
    for (const auto& row : rows) {
        users.push_back({row[0].value_or(""), row[1].value_or("")});
    }
    return users;
}

}  // namespace chatroom::storage::mysql
