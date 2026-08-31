#pragma once

#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

#include "chat/online_user_service.h"

namespace chatroom::chat {

/// 在线用户服务实现类：维护在线昵称集合，支持上线、下线和查询在线列表
class OnlineUserServiceImpl : public OnlineUserService {
private:
    /// 在线昵称集合
    std::unordered_set<std::string> online_users_;

    /// 互斥锁：多线程并发读写在线列表时保证数据一致
    mutable std::mutex mtx_;

public:
    void MarkOnline(const std::string& nickname) override;

    void MarkOffline(const std::string& nickname) override;

    std::vector<std::string> ListOnlineUsers() const override;
};

}  // namespace chatroom::chat
