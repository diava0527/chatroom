#pragma once

#include <string>
#include <vector>

#include "models/lobby_session.h"
#include "models/message.h"

namespace chatroom::storage {

class LobbyMemoryStore {
public:
    virtual ~LobbyMemoryStore() = default;

    // 1)代码逻辑：记录用户进入大厅的会话信息，主要保存昵称和进入时间点。
    // 2)返回值类型：void，原因是这里只定义大厅会话记录动作，供进入大厅接口调用。
    // 3)参数类型：const chatroom::models::LobbySession&，原因是进入大厅需要保存完整大厅会话信息，参数直接对应大厅历史边界需求。
    virtual void SaveLobbySession(const chatroom::models::LobbySession& session) = 0;

    // 1)代码逻辑：向大厅消息容器追加一条新消息，供消息广播和可见历史查询使用。
    // 2)返回值类型：void，原因是这里只定义内存追加动作，不需要同步返回额外业务对象，供大厅发言接口调用。
    // 3)参数类型：const chatroom::models::Message&，原因是大厅消息需要保留完整消息结构用于广播和历史展示。
    virtual void AppendLobbyMessage(const chatroom::models::Message& message) = 0;  

    // 1)代码逻辑：按进入大厅时间筛选当前用户可见的大厅历史，只返回进入后消息。
    // 2)返回值类型：std::vector<chatroom::models::Message>，原因是历史消息需要以前端可直接展示的列表形式返回，供大厅历史接口调用。
    // 3)参数类型：const std::string& enteredAt，原因是大厅历史显示边界由进入时间决定，参数直接对应需求限制。
    virtual std::vector<chatroom::models::Message> ListMessagesAfter(const std::string& enteredAt) const = 0;
};

}  // namespace chatroom::storage
