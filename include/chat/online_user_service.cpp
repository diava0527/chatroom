// 在线用户服务实现：对应 include/chat/online_user_service.h 接口。
// 当前仅搭好骨架，方法体为占位实现，具体业务逻辑待补充。
#include "chat/online_user_service.h"

namespace chatroom::chat {

// 在线用户服务的具体实现类。
class OnlineUserServiceImpl : public OnlineUserService {
public:
    // 标记上线：将用户登记为在线状态。
    void MarkOnline(const std::string& nickname) override {
        // TODO: 实现标记上线逻辑
        (void)nickname;
    }

    // 标记下线：移除用户的在线状态。
    void MarkOffline(const std::string& nickname) override {
        // TODO: 实现标记下线逻辑
        (void)nickname;
    }

    // 获取在线用户列表：返回当前在线昵称列表。
    std::vector<std::string> ListOnlineUsers() const override {
        // TODO: 实现在线用户列表逻辑
        return {};
    }
};

}  // namespace chatroom::chat
