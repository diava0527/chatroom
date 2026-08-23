// 大厅聊天服务实现：对应 include/chat/lobby_service.h 接口。
// 当前仅搭好骨架，方法体为占位实现，具体业务逻辑待补充。
#include "chat/lobby_service.h"

namespace chatroom::chat {

// 大厅聊天服务的具体实现类。
class LobbyServiceImpl : public LobbyService {
public:
    // 进入大厅：记录用户进入时间，作为历史可见边界。
    std::string EnterLobby(const std::string& nickname) override {
        // TODO: 实现进入大厅逻辑
        (void)nickname;
        return {};
    }

    // 发送大厅消息：构造并保存消息，返回完整消息体。
    chatroom::models::Message SendLobbyMessage(
        const std::string& senderNickname,
        const std::string& content) override {
        // TODO: 实现发送大厅消息逻辑
        (void)senderNickname;
        (void)content;
        return {};
    }

    // 拉取可见历史：返回进入时间之后的消息列表。
    std::vector<chatroom::models::Message> PullVisibleHistory(
        const std::string& enteredAt) const override {
        // TODO: 实现历史拉取逻辑
        (void)enteredAt;
        return {};
    }
};

}  // namespace chatroom::chat
