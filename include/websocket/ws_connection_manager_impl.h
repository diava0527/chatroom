#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "websocket/ws_connection_manager.h"

namespace chatroom::websocket {

// 通讯录管理员：维护「昵称 <-> 连接」的对应关系。
class WsConnectionManagerImpl : public WsConnectionManager {
public:
    WsConnectionManagerImpl() = default;

    void BindConnection(const std::string& nickname,
                        crow::websocket::connection& connection) override;
    void UnbindConnection(crow::websocket::connection& connection) override;
    bool SendToUser(const std::string& nickname,
                    const std::string& payload) override;
    void Broadcast(const std::string& payload) override;
    std::vector<std::string> ListConnectedUsers() const override;

private:
    // 正向：昵称 -> 连接（单播时用）
    std::unordered_map<std::string, crow::websocket::connection*> nickname_to_conn_;
    // 反向：连接 -> 昵称（挂断时反查用）
    std::unordered_map<crow::websocket::connection*, std::string> conn_to_nickname_;
    mutable std::mutex mtx_;
};

}  // namespace chatroom::websocket
