#include "websocket/ws_connection_manager_impl.h"

namespace chatroom::websocket {

void WsConnectionManagerImpl::BindConnection(const std::string& nickname,
                                             crow::websocket::connection& connection) {
    // 把 nickname 和 connection 双向记进两个 map
    std::lock_guard<std::mutex> lock(mtx_);
    const auto existing = nickname_to_conn_.find(nickname);
    if (existing != nickname_to_conn_.end()) {
        conn_to_nickname_.erase(existing->second);
    }
    nickname_to_conn_[nickname] = &connection;
    conn_to_nickname_[&connection] = nickname;
}

void WsConnectionManagerImpl::UnbindConnection(crow::websocket::connection& connection) {
    // 根据 connection 反查 nickname，再把两个 map 里的记录都删掉
    std::lock_guard<std::mutex> lock (mtx_);
    if (conn_to_nickname_.count(&connection) == 0) {
        return;
    }
    auto cur = conn_to_nickname_[&connection];
    conn_to_nickname_.erase (&connection);
    const auto forward = nickname_to_conn_.find(cur);
    if (forward != nickname_to_conn_.end()
        && forward->second == &connection) {
        nickname_to_conn_.erase(forward);
    }

}

bool WsConnectionManagerImpl::SendToUser(const std::string& nickname,
                                         const std::string& payload) {
    // 在 nickname_to_conn_ 里找到 connection，调用 send_text 发送，返回是否成功
    std::lock_guard<std::mutex> lock (mtx_);
    if (nickname_to_conn_.count (nickname) == 0) {
        return false;
    }
    auto connection = nickname_to_conn_[nickname];
    connection->send_text (payload);
    return true;
}

void WsConnectionManagerImpl::Broadcast(const std::string& payload) {
    // 遍历所有 connection，逐个 send_text
    std::lock_guard<std::mutex> lock (mtx_);
    for (const auto& [con, str] : conn_to_nickname_) {
        con->send_text (payload);
    }
}

std::vector<std::string> WsConnectionManagerImpl::ListConnectedUsers() const {
    // 返回所有在线昵称
    std::lock_guard<std::mutex> lock (mtx_);
    std::vector<std::string> ans;
    for (const auto &[string, con] : nickname_to_conn_) {
        ans.push_back (string);
    }
    return ans;
}

}  // namespace chatroom::websocket
