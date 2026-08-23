// 聊天 HTTP 控制器实现：对应 include/chat/chat_controller.h 接口。
// 当前仅搭好骨架，方法体为占位实现，具体业务逻辑待补充。
#include "chat/chat_controller.h"

namespace chatroom::chat {

// 聊天 HTTP 控制器的具体实现类。
class ChatControllerImpl : public ChatController {
public:
    // 创建私聊窗口：处理 HTTP 请求，校验身份后创建私聊会话。
    crow::response CreatePrivateSession(const crow::request& request) override {
        // TODO: 实现创建私聊窗口 HTTP 接口逻辑
        (void)request;
        return crow::response();
    }

    // 在线用户列表：处理 HTTP 请求，返回当前在线用户。
    crow::response ListOnlineUsers(const crow::request& request) override {
        // TODO: 实现在线用户列表 HTTP 接口逻辑
        (void)request;
        return crow::response();
    }
};

}  // namespace chatroom::chat
