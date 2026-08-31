#pragma once

#include <memory>

#include "storage/mysql/mysql_database.h"
#include "storage/private_chat_memory_store.h"

namespace chatroom::storage::mysql {

class MySqlPrivateChatStore final : public storage::PrivateChatMemoryStore {
public:
    explicit MySqlPrivateChatStore(std::shared_ptr<MySqlDatabase> database);

    chatroom::models::PrivateChatSession CreateSession(
        const std::string& senderNickname,
        const std::string& receiverNickname) override;
    std::optional<chatroom::models::PrivateChatSession> FindSession(
        const std::string& privateSessionId) const override;
    bool AppendMessage(const std::string& privateSessionId,
                       const chatroom::models::Message& message) override;
    std::vector<chatroom::models::Message> ListMessages(
        const std::string& privateSessionId) const override;
    void RemoveSessionsByUser(const std::string& nickname) override;

private:
    std::shared_ptr<MySqlDatabase> database_;
};

}  // namespace chatroom::storage::mysql
