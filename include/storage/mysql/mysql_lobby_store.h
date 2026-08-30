#pragma once

#include <memory>
#include <optional>

#include "storage/lobby_memory_store.h"
#include "storage/mysql/mysql_database.h"

namespace chatroom::storage::mysql {

class MySqlLobbyStore final : public storage::LobbyMemoryStore {
public:
    explicit MySqlLobbyStore(std::shared_ptr<MySqlDatabase> database);

    void SaveLobbySession(
        const chatroom::models::LobbySession& session) override;
    std::optional<std::string> FindEnteredAt(
        const std::string& nickname) const override;
    void AppendLobbyMessage(const chatroom::models::Message& message) override;
    std::vector<chatroom::models::Message> ListMessagesAfter(
        const std::string& enteredAt) const override;

private:
    std::shared_ptr<MySqlDatabase> database_;
};

}  // namespace chatroom::storage::mysql
