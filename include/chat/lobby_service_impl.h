#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "chat/lobby_service.h"
#include "storage/lobby_memory_store.h"

namespace chatroom::chat {

/// 大厅聊天服务实现类：负责记录进入大厅时间、构造并保存大厅消息、按时间边界拉取历史
class LobbyServiceImpl : public LobbyService {
private:
    /// 大厅存储：通过依赖注入传入，本类只负责造数据与转发，不直接保存数据
    std::shared_ptr<storage::LobbyMemoryStore> store_;

    /// 自增计数器：用于生成全局唯一消息 ID，原子操作保证多线程安全
    std::atomic<std::uint64_t> id_counter_{0};

    /// 生成全局唯一消息 ID（形如 "msg_1"、"msg_2"）
    std::string GenerateMessageId();

    /// 生成时间戳（格式 "YYYY-MM-DD HH:MM:SS"，定宽补零保证字符串比较等价于时间先后）
    std::string GenerateTimestamp() const;

public:
    /// 构造函数：注入大厅存储依赖。
    explicit LobbyServiceImpl(std::shared_ptr<storage::LobbyMemoryStore> store);

    std::string EnterLobby(const std::string& nickname) override;

    chatroom::models::Message SendLobbyMessage(
        const std::string& senderNickname, const std::string& content) override;

    std::vector<chatroom::models::Message> PullVisibleHistory(
        const std::string& enteredAt) const override;
};

}  // namespace chatroom::chat
