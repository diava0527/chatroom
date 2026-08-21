#pragma once

#include <string>
#include <vector>

#include "models/message.h"

namespace chatroom::chat {

class LobbyService {
public:
    virtual ~LobbyService() = default;

    // 1)代码逻辑：记录用户进入大厅的时间点，并建立“该用户当前可见大厅历史”的边界。
    // 2)返回值类型：std::string，原因是进入大厅后需要返回进入时间给上层作为历史查询边界，供 WebSocket 大厅进入接口调用。
    // 3)参数类型：const std::string& nickname，原因是进入大厅动作必须绑定到具体用户，参数直接对应大厅会话需求。
    virtual std::string EnterLobby(const std::string& nickname) = 0;

    // 1)代码逻辑：构造并保存一条大厅消息，供实时广播和后续大厅历史查询使用。
    // 2)返回值类型：chatroom::models::Message，原因是广播层需要完整消息体继续下发给大厅用户，供大厅发言接口调用。
    // 3)参数类型：const std::string& senderNickname 与 const std::string& content，原因是大厅发言最基本输入就是发送者和消息内容，参数直接对应大厅聊天要求。
    virtual chatroom::models::Message SendLobbyMessage(const std::string& senderNickname, const std::string& content) = 0;

    // 1)代码逻辑：根据用户进入大厅的时间边界，拉取其当前可见的大厅历史消息。
    // 2)返回值类型：std::vector<chatroom::models::Message>，原因是大厅历史前端需要按列表展示，供大厅历史接口调用。
    // 3)参数类型：const std::string& enteredAt，原因是历史边界完全由进入大厅的时刻决定，参数直接对应“不显示进入前消息”的要求。
    virtual std::vector<chatroom::models::Message> PullVisibleHistory(const std::string& enteredAt) const = 0;
};

}  // namespace chatroom::chat
