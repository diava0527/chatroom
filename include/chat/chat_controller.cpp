// 聊天 HTTP 控制器实现：对应 include/chat/chat_controller_impl.h 头文件
#include "chat/chat_controller_impl.h"
#include "common/constants.h"

#include <algorithm>
#include <optional>
#include <utility>

namespace {

// 文档 http_api.md 规定：sessionId 通过请求头 X-Session-Id 传递
constexpr const char* kSessionIdHeader = "X-Session-Id";
// 文档 http_api.md 规定：创建私聊窗口时，目标昵称字段名为 targetNickname
constexpr const char* kTargetNicknameField = "targetNickname";

}  // namespace

namespace chatroom::chat {

// 判断某个昵称是否在当前在线列表中
bool ChatControllerImpl::IsOnline(const std::vector<std::string>& onlineUsers,
                                  const std::string& nickname) const {
    return std::find(onlineUsers.begin(), onlineUsers.end(), nickname)
           != onlineUsers.end();
}

// 构造函数：注入鉴权、私聊、在线用户、响应构建四个依赖
ChatControllerImpl::ChatControllerImpl(
    std::shared_ptr<user::AuthService> auth_service,
    std::shared_ptr<PrivateChatService> private_chat_service,
    std::shared_ptr<OnlineUserService> online_user_service,
    std::shared_ptr<framework::ResponseBuilder> response_builder)
    : auth_service_(std::move(auth_service)),
      private_chat_service_(std::move(private_chat_service)),
      online_user_service_(std::move(online_user_service)),
      response_builder_(std::move(response_builder)) {}


// 创建私聊窗口：处理 HTTP 请求，校验身份后创建私聊会话
crow::response ChatControllerImpl::CreatePrivateSession(const crow::request& request) {
    // 1. 鉴权：从请求头 X-Session-Id 取 sessionId，反查当前用户
    std::string sessionId = request.get_header_value(kSessionIdHeader);
    auto currentUser = auth_service_->ValidateSession(sessionId);
    if (!currentUser.has_value()) {
        return response_builder_->BuildHttpJson(
            chatroom::common::kSessionInvalidCode, "session invalid", std::nullopt);
    }
    const std::string& senderNickname = currentUser.value();

    // 2. 解析请求体，取出目标昵称targetNickname
    auto body = crow::json::load(request.body);
    if (body.error() || !body.has(kTargetNicknameField)
        || body[kTargetNicknameField].t() != crow::json::type::String) {
        return response_builder_->BuildHttpJson(
            chatroom::common::kInvalidParamCode, "missing targetNickname", std::nullopt);
    }
    std::string targetNickname = body[kTargetNicknameField].s();

    if (targetNickname.empty() || targetNickname == senderNickname) {
        return response_builder_->BuildHttpJson(
            chatroom::common::kInvalidParamCode,
            "invalid targetNickname",
            std::nullopt);
    }

    // 3. 检查目标用户是否在线
    std::vector<std::string> onlineUsers = online_user_service_->ListOnlineUsers();
    if (!IsOnline(onlineUsers, targetNickname)) {
        return response_builder_->BuildHttpJson(
            chatroom::common::kTargetNotOnlineCode, "target user not online", std::nullopt);
    }

    // 4. 创建私聊窗口
    auto session = private_chat_service_->CreatePrivateSession(
        senderNickname, targetNickname);

    // 5. 返回data
    crow::json::wvalue data;
    data["privateSessionId"] = session.privateSessionId;
    data["senderNickname"] = session.senderNickname;
    data["receiverNickname"] = session.receiverNickname;
    return response_builder_->BuildHttpJson(
        chatroom::common::kSuccessCode, "private session created",
        std::optional<std::string>(data.dump()));
}

// 在线用户列表：处理HTTP请求，返回当前在线用户
crow::response ChatControllerImpl::ListOnlineUsers(const crow::request& request) {
    // 1. 鉴权
    std::string sessionId = request.get_header_value(kSessionIdHeader);
    auto currentUser = auth_service_->ValidateSession(sessionId);
    if (!currentUser.has_value()) {
        return response_builder_->BuildHttpJson(
            chatroom::common::kSessionInvalidCode, "session invalid", std::nullopt);
    }

    // 2. 取在线列表
    std::vector<std::string> users = online_user_service_->ListOnlineUsers();

    // 3. 包成json
    crow::json::wvalue data;
    data["onlineUsers"] = users;

    return response_builder_->BuildHttpJson(
        chatroom::common::kSuccessCode, "success",
        std::optional<std::string>(data.dump()));
}

}  // namespace chatroom::chat
