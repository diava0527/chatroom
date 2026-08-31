// 私聊服务实现：对应 include/chat/private_chat_service_impl.h 头文件
#include "chat/private_chat_service_impl.h"
#include "common/random_id.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>

namespace chatroom::chat {

// 生成全局唯一随机消息 ID，避免服务重启后与数据库旧记录冲突。
std::string PrivateChatServiceImpl::GenerateMessageId() {
    return chatroom::common::GenerateRandomHexId();
}

// 生成时间戳：格式为 "YYYY-MM-DD HH:MM:SS"，每个字段定宽补零，字符串比较等价于时间先后
std::string PrivateChatServiceImpl::GenerateTimestamp() const {
    using namespace std::chrono;
    std::time_t t = system_clock::to_time_t(system_clock::now());
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// 构造函数：注入私聊存储依赖
PrivateChatServiceImpl::PrivateChatServiceImpl(std::shared_ptr<storage::PrivateChatMemoryStore> store)
    : store_(std::move(store)) {
}

// 创建私聊窗口：建立双方用户之间的当前私聊会话
chatroom::models::PrivateChatSession PrivateChatServiceImpl::CreatePrivateSession(
    const std::string& senderNickname,
    const std::string& receiverNickname) {
    return store_->CreateSession(senderNickname, receiverNickname);
}

// 发送私聊消息：向指定窗口写入消息，返回完整消息体
chatroom::models::Message PrivateChatServiceImpl::SendPrivateMessage(
    const std::string& privateSessionId,
    const std::string& senderNickname,
    const std::string& content) {
    // 首先查询窗口，确认存在
    auto opt = store_->FindSession(privateSessionId);
    if (!opt.has_value()) {
        return {};  // 窗口不存在，返回空消息
    }
    const auto& session = opt.value();

    // 确定接收者：发送者是 sender 则接收者是 receiver，反之亦然
    std::string receiver;
    if (session.senderNickname == senderNickname) {
        receiver = session.receiverNickname;
    } else if (session.receiverNickname == senderNickname) {
        receiver = session.senderNickname;
    } else {
        return {};  // 发送者不属于这个窗口，请求非法，返回空消息
    }

    // 构造完整消息
    chatroom::models::Message msg;
    msg.messageId = GenerateMessageId();
    msg.senderNickname = senderNickname;
    msg.receiverNickname = receiver;
    msg.content = content;
    msg.timestamp = GenerateTimestamp();

    // 写入窗口（私聊追加消息）
    store_->AppendMessage(privateSessionId, msg);
    return msg;
}

// 拉取私聊历史：返回指定窗口的全部消息
std::vector<chatroom::models::Message> PrivateChatServiceImpl::PullPrivateHistory(
    const std::string& privateSessionId) const {
    return store_->ListMessages(privateSessionId);
}

bool PrivateChatServiceImpl::IsParticipant(
    const std::string& privateSessionId,
    const std::string& nickname) const {
    const auto session = store_->FindSession(privateSessionId);
    return session.has_value()
        && (session->senderNickname == nickname
            || session->receiverNickname == nickname);
}

// 清理用户会话：删除与该用户相关的全部私聊窗口
void PrivateChatServiceImpl::ClearSessionsByUser(const std::string& nickname) {
    store_->RemoveSessionsByUser(nickname);
}

}  // namespace chatroom::chat
