#pragma once

#include <memory>
#include <string>
#include <vector>

#include "chat/chat_controller.h"
#include "chat/online_user_service.h"
#include "chat/private_chat_service.h"
#include "framework/response_builder.h"
#include "user/auth_service.h"

namespace chatroom::chat {

/// 聊天 HTTP 控制器实现类：负责创建私聊窗口和获取在线用户列表两个HTTP接口的鉴权、转交与响应包装
class ChatControllerImpl : public ChatController {
private:
    /// 鉴权服务：校验 sessionId，反查当前登录用户昵称
    std::shared_ptr<user::AuthService> auth_service_;

    /// 私聊服务：创建私聊窗口
    std::shared_ptr<PrivateChatService> private_chat_service_;

    /// 在线用户服务：查询在线列表、判断目标用户是否在线
    std::shared_ptr<OnlineUserService> online_user_service_;

    /// 统一响应构建器：将业务结果包装成 {code, message, data} 格式
    std::shared_ptr<framework::ResponseBuilder> response_builder_;

    /// 判断某个昵称是否在当前在线列表中
    bool IsOnline(const std::vector<std::string>& onlineUsers,
                  const std::string& nickname) const;

public:
    /// 构造函数：注入鉴权、私聊、在线用户、响应构建四个依赖
    ChatControllerImpl(
        std::shared_ptr<user::AuthService> auth_service,
        std::shared_ptr<PrivateChatService> private_chat_service,
        std::shared_ptr<OnlineUserService> online_user_service,
        std::shared_ptr<framework::ResponseBuilder> response_builder);

    crow::response CreatePrivateSession(const crow::request& request) override;

    crow::response ListOnlineUsers(const crow::request& request) override;
};

}  // namespace chatroom::chat
