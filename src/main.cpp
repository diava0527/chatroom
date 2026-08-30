#define CROW_USE_BOOST 1

#include "crow_all.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

#include "chat/chat_protocol.h"
#include "chat/lobby_service_impl.h"
#include "chat/online_user_service_impl.h"
#include "chat/private_chat_service_impl.h"
#include "common/constants.h"
#include "framework/crow_router_registry.h"
#include "framework/crow_server_app.h"
#include "storage/lobby_memory_store_impl.h"
#include "storage/private_chat_memory_store_impl.h"
#include "storage/user_memory_store_impl.h"
#include "user/auth_service_impl.h"
#include "websocket/ws_connection_manager_impl.h"

namespace {

constexpr std::uint16_t kServerPort = 8080;
constexpr std::string_view kSessionHeader = "X-Session-Id";
constexpr std::string_view kFrontendRoot = "frontend";

std::filesystem::path ProjectRoot() {
    return std::filesystem::path(__FILE__).parent_path().parent_path();
}

std::optional<std::string> ReadTextFile(const std::filesystem::path& filePath) {
    std::ifstream input(filePath, std::ios::binary);
    if (!input) {
        return std::nullopt;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

crow::response BuildTextResponse(int statusCode,
                                 const std::string& body,
                                 const std::string& contentType) {
    crow::response response;
    response.code = statusCode;
    response.set_header("Content-Type", contentType);
    response.body = body;
    return response;
}

crow::response BuildJsonResponse(
    int code,
    const std::string& message,
    const std::function<void(crow::json::wvalue&)>& fillData = nullptr) {
    crow::json::wvalue responseBody;
    responseBody["code"] = code;
    responseBody["message"] = message;
    if (fillData) {
        fillData(responseBody["data"]);
    } else {
        responseBody["data"] = nullptr;
    }

    crow::response response{200, responseBody};
    response.set_header("Content-Type", "application/json; charset=utf-8");
    return response;
}

crow::response BuildFileResponse(const std::filesystem::path& root,
                                 const std::string& relativePath,
                                 const std::string& contentType) {
    const auto content = ReadTextFile(root / relativePath);
    if (!content.has_value()) {
        return BuildTextResponse(404, "file not found", "text/plain; charset=utf-8");
    }
    return BuildTextResponse(200, *content, contentType);
}

std::string BuildMainScript() {
    return R"JS(const sessionId = localStorage.getItem("sessionId");
const nickname = localStorage.getItem("nickname");
const HTTP_BASE = window.location.origin;
const WS_URL = `${window.location.protocol === "https:" ? "wss" : "ws"}://${window.location.host}/ws/chat?sessionId=${encodeURIComponent(sessionId || "")}`;

if (!sessionId || !nickname) {
    window.location.href = "login.html";
}

let ws = null;
let currentType = "lobby";
let currentUser = null;

const chatHistory = {
    lobby: []
};
const privateSessions = {};
const unreadPrivateCounts = {};

const userList = document.getElementById("userList");
const chatTitle = document.getElementById("chatTitle");
const chatBox = document.getElementById("chatBox");
const input = document.getElementById("messageInput");
const button = document.getElementById("sendButton");
const lobby = document.querySelector(".lobby");

function ensurePrivateHistory(username) {
    if (!chatHistory[username]) {
        chatHistory[username] = [];
    }
}

function renderMessages(messages) {
    chatBox.innerHTML = "";
    messages.forEach(function (message) {
        const wrapper = document.createElement("div");
        wrapper.className = "message-wrapper";
        wrapper.classList.add(message.senderNickname === nickname ? "message-self" : "message-other");

        const sender = document.createElement("div");
        sender.className = "message-sender";
        sender.textContent = message.senderNickname;

        const bubble = document.createElement("div");
        bubble.className = "message-bubble";
        bubble.textContent = message.content;

        const time = document.createElement("div");
        time.className = "message-time";
        time.textContent = message.timestamp || "";

        wrapper.appendChild(sender);
        wrapper.appendChild(bubble);
        wrapper.appendChild(time);
        chatBox.appendChild(wrapper);
    });
    chatBox.scrollTop = chatBox.scrollHeight;
}

function showMessages() {
    if (currentType === "lobby") {
        chatTitle.textContent = "公共大厅";
        renderMessages(chatHistory.lobby);
        return;
    }

    if (currentType === "private" && currentUser) {
        chatTitle.textContent = "和 " + currentUser + " 聊天";
        ensurePrivateHistory(currentUser);
        unreadPrivateCounts[currentUser] = 0;
        updateUserListFromState();
        renderMessages(chatHistory[currentUser]);
    }
}

function upsertPrivateMessage(privateSessionId, message) {
    const partner = message.senderNickname === nickname
        ? message.receiverNickname
        : message.senderNickname;

    if (!partner) {
        return;
    }

    privateSessions[partner] = privateSessionId;
    ensurePrivateHistory(partner);

    const duplicated = chatHistory[partner].some(function (item) {
        return item.messageId === message.messageId;
    });
    if (!duplicated) {
        chatHistory[partner].push(message);
    }

    if (currentType === "private" && currentUser === partner) {
        unreadPrivateCounts[partner] = 0;
        showMessages();
    } else if (message.senderNickname !== nickname) {
        unreadPrivateCounts[partner] = (unreadPrivateCounts[partner] || 0) + 1;
        updateUserListFromState();
    }
}

async function createPrivateSession(username) {
    const response = await fetch(`${HTTP_BASE}/api/v1/private-chat/session`, {
        method: "POST",
        headers: {
            "Content-Type": "application/json",
            "X-Session-Id": sessionId
        },
        body: JSON.stringify({
            targetNickname: username
        })
    });

    const result = await response.json();
    if (result.code !== 0) {
        throw new Error(result.message || "创建私聊失败");
    }

    privateSessions[username] = result.data.privateSessionId;
    ensurePrivateHistory(username);
    ws.send(JSON.stringify({
        event: "private.history.pull",
        payload: {
            privateSessionId: privateSessions[username]
        }
    }));
}

function connectWebSocket() {
    ws = new WebSocket(WS_URL);

    ws.onopen = function () {
        ws.send(JSON.stringify({ event: "lobby.enter" }));
        ws.send(JSON.stringify({ event: "lobby.history.pull" }));
    };

    ws.onmessage = function (event) {
        const data = JSON.parse(event.data);

        if (data.event === "lobby.enter.ack") {
            return;
        }

        if (data.event === "lobby.message.receive") {
            const message = data.payload;
            const duplicated = chatHistory.lobby.some(function (item) {
                return item.messageId === message.messageId;
            });
            if (!duplicated) {
                chatHistory.lobby.push(message);
            }
            if (currentType === "lobby") {
                showMessages();
            }
            return;
        }

        if (data.event === "lobby.history.response") {
            chatHistory.lobby = data.payload.messages || [];
            if (currentType === "lobby") {
                showMessages();
            }
            return;
        }

        if (data.event === "online.users.changed") {
            updateUserList(data.payload.users || []);
            return;
        }

        if (data.event === "private.message.receive") {
            upsertPrivateMessage(data.payload.privateSessionId, data.payload.message);
            return;
        }

        if (data.event === "private.history.response") {
            const privateSessionId = data.payload.privateSessionId;
            const messages = data.payload.messages || [];
            let partner = currentUser;
            if (!partner && messages.length > 0) {
                partner = messages[0].senderNickname === nickname
                    ? messages[0].receiverNickname
                    : messages[0].senderNickname;
            }
            if (partner) {
                privateSessions[partner] = privateSessionId;
                chatHistory[partner] = messages;
                if (currentType === "private" && currentUser === partner) {
                    showMessages();
                }
            }
        }
    };

    ws.onclose = function () {
        console.log("WebSocket closed");
    };
}

let latestOnlineUsers = [];

function updateUserListFromState() {
    userList.innerHTML = "";
    latestOnlineUsers.forEach(function (username) {
        if (username === nickname) {
            return;
        }

        const user = document.createElement("div");
        user.className = "user";
        user.dataset.user = username;

        const statusDot = document.createElement("span");
        statusDot.textContent = "🟢 ";
        user.appendChild(statusDot);

        const nameText = document.createElement("span");
        nameText.textContent = username;
        user.appendChild(nameText);

        const unreadCount = unreadPrivateCounts[username] || 0;
        if (unreadCount > 0) {
            const badge = document.createElement("span");
            badge.textContent = unreadCount > 99 ? "99+" : String(unreadCount);
            badge.style.cssText = "float:right;min-width:18px;height:18px;line-height:18px;padding:0 6px;margin-top:2px;border-radius:999px;background:#e53935;color:#fff;font-size:12px;text-align:center;";
            user.appendChild(badge);
        }

        user.addEventListener("click", async function () {
            currentType = "private";
            currentUser = username;
            ensurePrivateHistory(username);
            unreadPrivateCounts[username] = 0;

            try {
                if (!privateSessions[username]) {
                    await createPrivateSession(username);
                } else if (ws && ws.readyState === WebSocket.OPEN) {
                    ws.send(JSON.stringify({
                        event: "private.history.pull",
                        payload: {
                            privateSessionId: privateSessions[username]
                        }
                    }));
                }
            } catch (error) {
                alert(error.message || "创建私聊失败");
            }

            showMessages();
        });
        userList.appendChild(user);
    });
}

function updateUserList(users) {
    latestOnlineUsers = users.slice();
    updateUserListFromState();
}

lobby.addEventListener("click", function () {
    currentType = "lobby";
    currentUser = null;
    showMessages();
});

button.addEventListener("click", function () {
    const message = input.value.trim();
    if (!message) {
        return;
    }
    if (!ws || ws.readyState !== WebSocket.OPEN) {
        alert("聊天服务器还没有连接");
        return;
    }

    if (currentType === "lobby") {
        ws.send(JSON.stringify({
            event: "lobby.message.send",
            payload: { content: message }
        }));
    } else if (currentType === "private" && currentUser && privateSessions[currentUser]) {
        ws.send(JSON.stringify({
            event: "private.message.send",
            payload: {
                privateSessionId: privateSessions[currentUser],
                content: message
            }
        }));
    } else {
        alert("请先选择公共大厅或者一个用户");
        return;
    }

    input.value = "";
});

input.addEventListener("keydown", function (event) {
    if (event.key === "Enter" && !event.shiftKey) {
        event.preventDefault();
        button.click();
    }
});

showMessages();
connectWebSocket();
)JS";
}

void FillMessageJson(crow::json::wvalue& target, const chatroom::models::Message& message) {
    target["messageId"] = message.messageId;
    target["senderNickname"] = message.senderNickname;
    target["receiverNickname"] = message.receiverNickname;
    target["content"] = message.content;
    target["timestamp"] = message.timestamp;
}

}  // namespace

#include <exception>
#include <iostream>

int main() {
    const auto projectRoot = ProjectRoot();
    const auto frontendRoot = projectRoot / kFrontendRoot;

    auto userStore = std::make_shared<chatroom::storage::UserMemoryStoreImpl>();
    auto lobbyStore = std::make_shared<chatroom::storage::LobbyMemoryStoreImpl>();
    auto privateChatStore = std::make_shared<chatroom::storage::PrivateChatMemoryStoreImpl>();

    auto authService = std::make_shared<chatroom::user::AuthServiceImpl>(userStore);
    auto onlineUserService = std::make_shared<chatroom::chat::OnlineUserServiceImpl>();
    auto lobbyService = std::make_shared<chatroom::chat::LobbyServiceImpl>(lobbyStore);
    auto privateChatService =
        std::make_shared<chatroom::chat::PrivateChatServiceImpl>(privateChatStore);
    auto connectionManager =
        std::make_shared<chatroom::websocket::WsConnectionManagerImpl>();

    std::unordered_map<std::string, std::string> lobbyEnteredAt;
    std::mutex lobbyEnteredAtMutex;

    const auto broadcastOnlineUsers = [&]() {
        const auto users = onlineUserService->ListOnlineUsers();
        crow::json::wvalue response;
        response["event"] =
            std::string(chatroom::chat::ChatProtocol::kOnlineUsersChanged);
        for (size_t index = 0; index < users.size(); ++index) {
            response["payload"]["users"][index] = users[index];
        }
        connectionManager->Broadcast(response.dump());
    };

    chatroom::framework::CrowRouterRegistry routerRegistry;
    chatroom::framework::CrowServerApp serverApp(routerRegistry, kServerPort);

    routerRegistry.AddRegistrar([&](crow::SimpleApp& app) {
        CROW_ROUTE(app, "/")([&]() {
            return BuildFileResponse(frontendRoot, "login.html", "text/html; charset=utf-8");
        });

        CROW_ROUTE(app, "/login.html")([&]() {
            return BuildFileResponse(frontendRoot, "login.html", "text/html; charset=utf-8");
        });

        CROW_ROUTE(app, "/register.html")([&]() {
            return BuildFileResponse(frontendRoot, "register.html", "text/html; charset=utf-8");
        });

        CROW_ROUTE(app, "/index.html")([&]() {
            return BuildFileResponse(frontendRoot, "index.html", "text/html; charset=utf-8");
        });

        CROW_ROUTE(app, "/css/style.css")([&]() {
            return BuildFileResponse(frontendRoot, "css/style.css", "text/css; charset=utf-8");
        });

        CROW_ROUTE(app, "/js/login.js")([&]() {
            return BuildFileResponse(frontendRoot, "js/login.js", "application/javascript; charset=utf-8");
        });

        CROW_ROUTE(app, "/js/register.js")([&]() {
            return BuildFileResponse(frontendRoot, "js/register.js", "application/javascript; charset=utf-8");
        });

        CROW_ROUTE(app, "/js/main.js")([&]() {
            return BuildTextResponse(
                200, BuildMainScript(), "application/javascript; charset=utf-8");
        });

        CROW_ROUTE(app, "/api/v1/health")([]() {
            return BuildJsonResponse(chatroom::common::kSuccessCode, "server started");
        });

        CROW_ROUTE(app, "/api/v1/auth/register")
            .methods(crow::HTTPMethod::Post)([&](const crow::request& request) {
                auto body = crow::json::load(request.body);
                if (!body || !body.has("nickname") || !body.has("password")) {
                    return BuildJsonResponse(
                        chatroom::common::kInvalidParamCode, "invalid request");
                }

                const std::string nickname = body["nickname"].s();
                const std::string password = body["password"].s();
                if (nickname.empty() || password.empty()) {
                    return BuildJsonResponse(
                        chatroom::common::kInvalidParamCode, "invalid request");
                }

                if (!authService->Register(nickname, password)) {
                    return BuildJsonResponse(
                        chatroom::common::kNicknameExistsCode,
                        "nickname already exists");
                }

                return BuildJsonResponse(
                    chatroom::common::kSuccessCode,
                    "register success",
                    [&](crow::json::wvalue& data) { data["nickname"] = nickname; });
            });

        CROW_ROUTE(app, "/api/v1/auth/login")
            .methods(crow::HTTPMethod::Post)([&](const crow::request& request) {
                auto body = crow::json::load(request.body);
                if (!body || !body.has("nickname") || !body.has("password")) {
                    return BuildJsonResponse(
                        chatroom::common::kLoginFailedCode,
                        "nickname or password error");
                }

                const std::string nickname = body["nickname"].s();
                const std::string password = body["password"].s();
                auto sessionId = authService->Login(nickname, password);
                if (!sessionId.has_value()) {
                    return BuildJsonResponse(
                        chatroom::common::kLoginFailedCode,
                        "nickname or password error");
                }

                return BuildJsonResponse(
                    chatroom::common::kSuccessCode,
                    "login success",
                    [&](crow::json::wvalue& data) {
                        data["nickname"] = nickname;
                        data["sessionId"] = *sessionId;
                    });
            });

        CROW_ROUTE(app, "/api/v1/auth/logout")
            .methods(crow::HTTPMethod::Post)([&](const crow::request& request) {
                const std::string sessionId =
                    request.get_header_value(std::string(kSessionHeader));
                auto nickname = authService->ValidateSession(sessionId);
                if (!nickname.has_value()) {
                    return BuildJsonResponse(
                        chatroom::common::kSessionInvalidCode,
                        "sessionId invalid");
                }

                authService->Logout(sessionId);
                onlineUserService->MarkOffline(*nickname);
                privateChatService->ClearSessionsByUser(*nickname);
                {
                    std::lock_guard<std::mutex> lock(lobbyEnteredAtMutex);
                    lobbyEnteredAt.erase(*nickname);
                }
                broadcastOnlineUsers();

                return BuildJsonResponse(
                    chatroom::common::kSuccessCode, "logout success");
            });

        CROW_ROUTE(app, "/api/v1/users/online")
            .methods(crow::HTTPMethod::Get)([&](const crow::request& request) {
                const std::string sessionId =
                    request.get_header_value(std::string(kSessionHeader));
                if (!authService->ValidateSession(sessionId).has_value()) {
                    return BuildJsonResponse(
                        chatroom::common::kSessionInvalidCode, "session invalid");
                }

                const auto users = onlineUserService->ListOnlineUsers();
                return BuildJsonResponse(
                    chatroom::common::kSuccessCode,
                    "success",
                    [&](crow::json::wvalue& data) {
                        for (size_t index = 0; index < users.size(); ++index) {
                            data["onlineUsers"][index] = users[index];
                        }
                    });
            });

        CROW_ROUTE(app, "/api/v1/private-chat/session")
            .methods(crow::HTTPMethod::Post)([&](const crow::request& request) {
                const std::string sessionId =
                    request.get_header_value(std::string(kSessionHeader));
                auto currentUser = authService->ValidateSession(sessionId);
                if (!currentUser.has_value()) {
                    return BuildJsonResponse(
                        chatroom::common::kSessionInvalidCode, "session invalid");
                }

                auto body = crow::json::load(request.body);
                if (!body || !body.has("targetNickname")) {
                    return BuildJsonResponse(
                        chatroom::common::kInvalidParamCode,
                        "missing targetNickname");
                }

                const std::string targetNickname = body["targetNickname"].s();
                const auto onlineUsers = onlineUserService->ListOnlineUsers();
                const bool targetOnline =
                    std::find(onlineUsers.begin(), onlineUsers.end(), targetNickname)
                    != onlineUsers.end();
                if (!targetOnline || targetNickname == *currentUser) {
                    return BuildJsonResponse(
                        chatroom::common::kTargetNotOnlineCode,
                        "target user not online");
                }

                const auto session =
                    privateChatService->CreatePrivateSession(*currentUser, targetNickname);
                return BuildJsonResponse(
                    chatroom::common::kSuccessCode,
                    "private session created",
                    [&](crow::json::wvalue& data) {
                        data["privateSessionId"] = session.privateSessionId;
                        data["senderNickname"] = session.senderNickname;
                        data["receiverNickname"] = session.receiverNickname;
                    });
            });

        CROW_WEBSOCKET_ROUTE(app, "/ws/chat")
            .onaccept([&](const crow::request& request,
                          std::optional<crow::response>& response,
                          void** userData) {
                const char* rawSessionId = request.url_params.get("sessionId");
                const std::string sessionId = rawSessionId == nullptr ? "" : rawSessionId;
                auto nickname = authService->ValidateSession(sessionId);
                if (!nickname.has_value()) {
                    response = BuildTextResponse(
                        401, "invalid session", "text/plain; charset=utf-8");
                    return;
                }

                *userData = new std::string(*nickname);
            })
            .onopen([&](crow::websocket::connection& connection) {
                auto* nicknamePtr =
                    static_cast<std::string*>(connection.userdata());
                if (nicknamePtr == nullptr) {
                    connection.close();
                    return;
                }

                connectionManager->BindConnection(*nicknamePtr, connection);
                onlineUserService->MarkOnline(*nicknamePtr);
                broadcastOnlineUsers();
            })
            .onmessage([&](crow::websocket::connection& connection,
                           const std::string& rawMessage,
                           bool /*isBinary*/) {
                auto* nicknamePtr =
                    static_cast<std::string*>(connection.userdata());
                if (nicknamePtr == nullptr) {
                    return;
                }

                auto message = crow::json::load(rawMessage);
                if (!message || !message.has("event")) {
                    return;
                }

                const std::string event = message["event"].s();
                const std::string& nickname = *nicknamePtr;

                if (event == chatroom::chat::ChatProtocol::kLobbyEnter) {
                    const std::string enteredAt = lobbyService->EnterLobby(nickname);
                    {
                        std::lock_guard<std::mutex> lock(lobbyEnteredAtMutex);
                        lobbyEnteredAt[nickname] = enteredAt;
                    }

                    crow::json::wvalue response;
                    response["event"] =
                        std::string(chatroom::chat::ChatProtocol::kLobbyEnterAck);
                    response["payload"]["nickname"] = nickname;
                    response["payload"]["enteredAt"] = enteredAt;
                    connection.send_text(response.dump());
                    return;
                }

                if (event == chatroom::chat::ChatProtocol::kLobbyHistoryPull) {
                    std::string enteredAt;
                    {
                        std::lock_guard<std::mutex> lock(lobbyEnteredAtMutex);
                        enteredAt = lobbyEnteredAt[nickname];
                    }

                    const auto messages = lobbyService->PullVisibleHistory(enteredAt);
                    crow::json::wvalue response;
                    response["event"] =
                        std::string(chatroom::chat::ChatProtocol::kLobbyHistoryResponse);
                    for (size_t index = 0; index < messages.size(); ++index) {
                        FillMessageJson(response["payload"]["messages"][index], messages[index]);
                    }
                    connection.send_text(response.dump());
                    return;
                }

                if (event == chatroom::chat::ChatProtocol::kLobbySend) {
                    if (!message.has("payload") || !message["payload"].has("content")) {
                        return;
                    }

                    const auto sentMessage = lobbyService->SendLobbyMessage(
                        nickname, message["payload"]["content"].s());
                    crow::json::wvalue response;
                    response["event"] =
                        std::string(chatroom::chat::ChatProtocol::kLobbyReceive);
                    FillMessageJson(response["payload"], sentMessage);
                    connectionManager->Broadcast(response.dump());
                    return;
                }

                if (event == chatroom::chat::ChatProtocol::kPrivateHistoryPull) {
                    if (!message.has("payload")
                        || !message["payload"].has("privateSessionId")) {
                        return;
                    }

                    const std::string privateSessionId =
                        message["payload"]["privateSessionId"].s();
                    const auto messages =
                        privateChatService->PullPrivateHistory(privateSessionId);

                    crow::json::wvalue response;
                    response["event"] = std::string(
                        chatroom::chat::ChatProtocol::kPrivateHistoryResponse);
                    response["payload"]["privateSessionId"] = privateSessionId;
                    for (size_t index = 0; index < messages.size(); ++index) {
                        FillMessageJson(response["payload"]["messages"][index], messages[index]);
                    }
                    connection.send_text(response.dump());
                    return;
                }

                if (event == chatroom::chat::ChatProtocol::kPrivateSend) {
                    if (!message.has("payload")
                        || !message["payload"].has("privateSessionId")
                        || !message["payload"].has("content")) {
                        return;
                    }

                    const std::string privateSessionId =
                        message["payload"]["privateSessionId"].s();
                    const auto sentMessage = privateChatService->SendPrivateMessage(
                        privateSessionId,
                        nickname,
                        message["payload"]["content"].s());
                    if (sentMessage.messageId.empty()) {
                        return;
                    }

                    crow::json::wvalue response;
                    response["event"] =
                        std::string(chatroom::chat::ChatProtocol::kPrivateReceive);
                    response["payload"]["privateSessionId"] = privateSessionId;
                    FillMessageJson(response["payload"]["message"], sentMessage);
                    const std::string payload = response.dump();
                    connection.send_text(payload);
                    connectionManager->SendToUser(sentMessage.receiverNickname, payload);
                }
            })
            .onclose([&](crow::websocket::connection& connection,
                         const std::string& /*reason*/,
                         uint16_t /*closeCode*/) {
                auto* nicknamePtr =
                    static_cast<std::string*>(connection.userdata());
                if (nicknamePtr == nullptr) {
                    return;
                }

                connectionManager->UnbindConnection(connection);
                onlineUserService->MarkOffline(*nicknamePtr);
                {
                    std::lock_guard<std::mutex> lock(lobbyEnteredAtMutex);
                    lobbyEnteredAt.erase(*nicknamePtr);
                }
                broadcastOnlineUsers();
                delete nicknamePtr;
            });
    });

    serverApp.Run();
    return 0;
}
