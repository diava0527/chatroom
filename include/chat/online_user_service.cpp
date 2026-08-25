// 在线用户服务实现：对应 include/chat/online_user_service_impl.h 头文件
#include "chat/online_user_service_impl.h"

namespace chatroom::chat {

// 标记上线：将用户登记为在线状态
void OnlineUserServiceImpl::MarkOnline(const std::string& nickname) {
    std::lock_guard<std::mutex> lock(mtx_);
    online_users_.insert(nickname);
}

// 标记下线：移除用户的在线状态
void OnlineUserServiceImpl::MarkOffline(const std::string& nickname) {
    std::lock_guard<std::mutex> lock(mtx_);
    online_users_.erase(nickname);
}

// 获取在线用户列表：返回当前在线昵称列表（暂未排序，可按需加入）
std::vector<std::string> OnlineUserServiceImpl::ListOnlineUsers() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return std::vector<std::string>(online_users_.begin(), online_users_.end());
}

}  // namespace chatroom::chat
