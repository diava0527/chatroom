#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "user/auth_service_impl.h"
#include "websocket/ws_chat_controller_impl.h"
#include "websocket/ws_connection_manager_impl.h"
#include "chat/online_user_service_impl.h"

namespace {
void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Users final : public chatroom::storage::UserMemoryStore {
    std::vector<chatroom::models::User> users_;
public:
    bool SaveUser(const chatroom::models::User& user) override {
        if (FindUserByNickname(user.nickname)) return false;
        users_.push_back(user); return true;
    }
    std::optional<chatroom::models::User> FindUserByNickname(const std::string& nickname) const override {
        for (const auto& user : users_) if (user.nickname == nickname) return user;
        return std::nullopt;
    }
    std::vector<chatroom::models::User> ListUsers() const override { return users_; }
};

class Connection final : public crow::websocket::connection {
public:
    Connection() { userdata(nullptr); }
    bool closed = false;
    void send_binary(std::string) override {}
    void send_text(std::string) override {}
    void send_ping(std::string) override {}
    void send_pong(std::string) override {}
    void close(const std::string&, uint16_t) override { closed = true; }
    std::string get_remote_ip() override { return "127.0.0.1"; }
    std::string get_subprotocol() const override { return {}; }
};

class Dispatcher final : public chatroom::websocket::WsMessageDispatcher {
public:
    int messages = 0;
    void Dispatch(crow::websocket::connection&, const std::string&) override { ++messages; }
};
}

int main() {
    try {
        using namespace std::chrono;
        using Auth = chatroom::user::AuthServiceImpl;
        auto time = Auth::Clock::time_point{};
        std::vector<std::string> ended;
        auto auth = std::make_shared<Auth>(std::make_shared<Users>(),
            [&](const std::string& name) { ended.push_back(name); }, [&] { return time; });
        auto online = std::make_shared<chatroom::chat::OnlineUserServiceImpl>();
        auto manager = std::make_shared<chatroom::websocket::WsConnectionManagerImpl>();
        auto dispatcher = std::make_shared<Dispatcher>();
        chatroom::websocket::WsChatControllerImpl controller(auth, online, manager, dispatcher);
        Check(auth->Register("n", "test-password"), "register");
        const auto sid = auth->Login("n", "test-password").value();
        Check(!auth->Login("n", "test-password"), "pending handshake reserves login briefly");
        Connection original;
        controller.OnOpen(original, sid);
        Check(!auth->Login("n", "test-password"), "active user rejects duplicate login");
        controller.OnMessage(original, "hello");
        Check(dispatcher->messages == 1, "active connection dispatches");

        // 直接关网页，无 HTTP logout，立即允许用密码登录。
        controller.OnClose(original);
        Check(online->ListOnlineUsers().empty(), "closed page is offline");
        Check(!auth->Login("n", "wrong"), "offline login still verifies password");
        Check(auth->ValidateSession(sid).has_value(), "wrong password does not revoke old session");
        const auto replacement = auth->Login("n", "test-password").value();
        Check(sid != replacement && !auth->ValidateSession(sid), "closed page can immediately login; old token revoked");
        Check(ended.size() == 1, "relogin clears old private sessions once");
        Connection reopened;
        controller.OnOpen(reopened, replacement);

        // 刷新/短暂断网/小程序后台恢复时继续使用同一个 session。
        controller.OnClose(reopened);
        time += seconds(299);
        auth->ExpireDisconnectedSessions();
        Check(auth->ValidateSession(replacement).has_value(), "reconnect grace retains session");
        Connection resumed;
        controller.OnOpen(resumed, replacement);
        time += hours(1);
        auth->ExpireDisconnectedSessions();
        Check(auth->ValidateSession(replacement).has_value(), "active connection does not expire");
        Check(ended.size() == 1, "reconnect does not clear private history");

        // 新连接建立后，旧连接迟到的 close/message 均不能影响新连接。
        Connection newer;
        controller.OnOpen(newer, replacement);
        controller.OnMessage(resumed, "stale");
        Check(resumed.closed && dispatcher->messages == 1, "stale connection cannot send");
        controller.OnClose(resumed);
        Check(!online->ListOnlineUsers().empty(), "stale close cannot mark current user offline");
        Check(!auth->Login("n", "test-password"), "stale close cannot release active login");
        controller.OnMessage(newer, "current");
        Check(dispatcher->messages == 2, "new connection still dispatches");

        controller.OnClose(newer);
        time += seconds(300);
        Check(!auth->ValidateSession(replacement), "expired offline session rejects HTTP auth");
        auth->ExpireDisconnectedSessions();
        Check(ended.size() == 2, "timer clears abandoned private history");
        Connection expired;
        controller.OnOpen(expired, replacement);
        Check(expired.closed, "expired websocket rejected");
        controller.OnClose(expired);

        const auto pending = auth->Login("n", "test-password").value();
        time += seconds(15);
        auth->ExpireDisconnectedSessions();
        Check(!auth->ValidateSession(pending), "login without websocket eventually reclaimed");
        const auto finalId = auth->Login("n", "test-password").value();
        Connection finalConnection;
        controller.OnOpen(finalConnection, finalId);
        Check(auth->Logout(finalId), "explicit logout");
        controller.OnMessage(finalConnection, "after logout");
        Check(finalConnection.closed && dispatcher->messages == 2, "logged out socket cannot send");
        controller.OnClose(finalConnection);
        Check(!auth->Logout(finalId), "old logout cannot revoke new session");
        std::cout << "PASS: close without logout, password verification, immediate relogin, refresh/background reconnect, "
                     "stale callbacks, offline expiry, pending handshake expiry, private cleanup and revoked socket\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n'; return 1;
    }
}
