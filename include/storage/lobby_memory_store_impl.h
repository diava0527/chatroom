#pragma once

#include <mutex>
<<<<<<< HEAD
=======
#include <optional>
>>>>>>> N-storage
#include <string>
#include <vector>

#include "storage/lobby_memory_store.h"

namespace chatroom::storage {

/// 大厅存储的内存实现：进程运行期间保存大厅会话和大厅消息。
class LobbyMemoryStoreImpl final : public LobbyMemoryStore {
public:
    /// 记录用户本次进入大厅的昵称和时间，用于保留进入时刻这一历史边界。
    void SaveLobbySession(const chatroom::models::LobbySession& session) override;

<<<<<<< HEAD
=======
    /// 按昵称返回该用户最早一次进入大厅的时间；无记录时返回空。
    std::optional<std::string> FindEnteredAt(
        const std::string& nickname) const override;

>>>>>>> N-storage
    /// 追加一条大厅广播消息，保持其写入顺序。
    void AppendLobbyMessage(const chatroom::models::Message& message) override;

    /// 返回时间戳严格晚于 enteredAt 的消息，不包含进入时刻之前或同一时刻的消息。
    std::vector<chatroom::models::Message> ListMessagesAfter(
        const std::string& enteredAt) const override;

private:
    /// 保护以下两个容器，支持 WebSocket 请求并发读写。
    mutable std::mutex mutex_;

    /// 已进入大厅的会话记录；当前用于保存用户进入时间。
    std::vector<chatroom::models::LobbySession> sessions_;

    /// 已发送的大厅消息，按追加顺序保存。
    std::vector<chatroom::models::Message> messages_;
};

}  // namespace chatroom::storage
