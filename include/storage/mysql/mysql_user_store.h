#pragma once

#include <memory>

#include "storage/mysql/mysql_database.h"
#include "storage/user_memory_store.h"

namespace chatroom::storage::mysql {

class MySqlUserStore final : public storage::UserMemoryStore {
public:
    explicit MySqlUserStore(std::shared_ptr<MySqlDatabase> database);

    bool SaveUser(const chatroom::models::User& user) override;
    std::optional<chatroom::models::User> FindUserByNickname(
        const std::string& nickname) const override;
    std::vector<chatroom::models::User> ListUsers() const override;

private:
    std::shared_ptr<MySqlDatabase> database_;
};

}  // namespace chatroom::storage::mysql
