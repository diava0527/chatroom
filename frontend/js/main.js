const sessionId = getSessionId();
const nickname = getNickname();

if (!sessionId || !nickname) {
    window.location.href = "login.html";
}

// ---------- DOM 引用 ----------

const connectionStatus = document.getElementById("connectionStatus");
const onlineCount = document.getElementById("onlineCount");
const userList = document.getElementById("userList");
const chatTitle = document.getElementById("chatTitle");
const chatBox = document.getElementById("chatBox");
const input = document.getElementById("messageInput");
const sendButton = document.getElementById("sendButton");
const lobbyButton = document.querySelector(".lobby");
const logoutButton = document.getElementById("logoutButton");
const currentNickname = document.getElementById("currentNickname");

currentNickname.textContent = nickname || "";

// ---------- 状态 ----------

let ws = null;
let reconnectTimer = null;
let reconnectAttempt = 0;
let manualClose = false;

let currentType = "lobby";
let currentUser = null;

const chatHistory = { lobby: [] };
const privateSessions = {};
const unread = { lobby: 0 };

// ---------- 工具函数 ----------

function ensurePrivateHistory(username) {
    if (!chatHistory[username]) {
        chatHistory[username] = [];
    }
}

function addMessageIfMissing(history, message) {
    const exists = history.some((item) => item.messageId === message.messageId);
    if (!exists) {
        history.push(message);
    }
    return !exists;
}

function formatTime(timestamp) {
    if (!timestamp) return "";
    const parts = String(timestamp).split(" ");
    return parts.length > 1 ? parts[1] : timestamp;
}

function findPartnerBySessionId(privateSessionId) {
    return Object.keys(privateSessions).find(
        (username) => privateSessions[username] === privateSessionId
    );
}

// ---------- 消息渲染 ----------

function renderMessages(messages) {
    chatBox.innerHTML = "";

    if (!messages || messages.length === 0) {
        const tip = document.createElement("div");
        tip.className = "empty-tip";
        tip.textContent =
            currentType === "lobby"
                ? "大厅还没有消息，来打个招呼吧"
                : "还没有消息，发送第一条吧";
        chatBox.appendChild(tip);
        return;
    }

    messages.forEach(function (message) {
        const wrapper = document.createElement("div");
        wrapper.className = "message-wrapper";
        wrapper.classList.add(
            message.senderNickname === nickname
                ? "message-self"
                : "message-other"
        );

        if (currentType === "lobby") {
            const sender = document.createElement("div");
            sender.className = "message-sender";
            sender.textContent = message.senderNickname;
            wrapper.appendChild(sender);
        }

        const bubble = document.createElement("div");
        bubble.className = "message-bubble";
        bubble.textContent = message.content;

        const time = document.createElement("div");
        time.className = "message-time";
        time.textContent = formatTime(message.timestamp);

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
        ensurePrivateHistory(currentUser);
        chatTitle.textContent = `和 ${currentUser} 聊天`;
        renderMessages(chatHistory[currentUser]);
    }
}

// ---------- 未读徽章 ----------

function incrementUnread(key) {
    if (unread[key] === undefined) unread[key] = 0;
    unread[key] += 1;
    renderBadges();
}

function clearUnread(key) {
    unread[key] = 0;
    renderBadges();
}

function renderBadges() {
    const lobbyBadge = document.getElementById("lobbyBadge");
    const lobbyCount = unread.lobby || 0;
    lobbyBadge.textContent = lobbyCount > 0 ? lobbyCount : "";
    lobbyBadge.classList.toggle("show", lobbyCount > 0);

    document.querySelectorAll(".user-list .user[data-user]").forEach((el) => {
        const name = el.dataset.user;
        const count = unread[name] || 0;
        let badge = el.querySelector(".badge");
        if (!badge) {
            badge = document.createElement("span");
            badge.className = "badge";
            el.appendChild(badge);
        }
        badge.textContent = count > 0 ? count : "";
        badge.classList.toggle("show", count > 0);
    });
}

// ---------- 会话切换 ----------

function setActiveDom(key) {
    document.querySelectorAll(".user-list .user").forEach((el) => {
        const match =
            key === "lobby"
                ? el.dataset.type === "lobby"
                : el.dataset.user === key;
        el.classList.toggle("active", match);
    });
}

function switchToLobby() {
    currentType = "lobby";
    currentUser = null;
    setActiveDom("lobby");
    clearUnread("lobby");
    showMessages();
}

async function openPrivateChat(username) {
    try {
        let privateSessionId = privateSessions[username];
        if (!privateSessionId) {
            privateSessionId = await createPrivateSession(username);
        }

        currentType = "private";
        currentUser = username;
        setActiveDom(username);
        clearUnread(username);
        showMessages();

        if (ws && ws.readyState === WebSocket.OPEN) {
            ws.send(
                JSON.stringify({
                    event: "private.history.pull",
                    payload: { privateSessionId },
                })
            );
        }
    } catch (error) {
        showToast(error.message || "无法创建私聊", "error");
    }
}

async function createPrivateSession(username) {
    const result = await apiFetch("/api/v1/private-chat/session", {
        method: "POST",
        auth: true,
        body: { targetNickname: username },
    });
    privateSessions[username] = result.data.privateSessionId;
    ensurePrivateHistory(username);
    return result.data.privateSessionId;
}

// ---------- WebSocket 消息处理 ----------

function handlePrivateMessage(privateSessionId, message) {
    const partner =
        message.senderNickname === nickname
            ? message.receiverNickname
            : message.senderNickname;

    if (!partner) {
        return;
    }

    privateSessions[partner] = privateSessionId;
    ensurePrivateHistory(partner);
    const isNew = addMessageIfMissing(chatHistory[partner], message);

    if (currentType === "private" && currentUser === partner) {
        showMessages();
    } else if (isNew) {
        incrementUnread(partner);
    }
}

function handleWebSocketMessage(data) {
    if (data.event === "lobby.enter.ack") {
        return;
    }

    if (data.event === "lobby.message.receive") {
        addMessageIfMissing(chatHistory.lobby, data.payload);
        if (currentType === "lobby") {
            showMessages();
        } else {
            incrementUnread("lobby");
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
        updateUserList((data.payload && data.payload.users) || []);
        return;
    }

    if (data.event === "private.message.receive") {
        handlePrivateMessage(
            data.payload.privateSessionId,
            data.payload.message
        );
        return;
    }

    if (data.event === "private.history.response") {
        const privateSessionId = data.payload.privateSessionId;
        let partner = findPartnerBySessionId(privateSessionId);
        const messages = data.payload.messages || [];

        if (!partner && messages.length > 0) {
            partner =
                messages[0].senderNickname === nickname
                    ? messages[0].receiverNickname
                    : messages[0].senderNickname;
            privateSessions[partner] = privateSessionId;
        }

        if (partner) {
            ensurePrivateHistory(partner);
            chatHistory[partner] = messages;
            if (currentType === "private" && currentUser === partner) {
                showMessages();
            }
        }
        return;
    }

    if (data.event === "chat.error") {
        showToast(data.payload.message || "聊天请求失败", "error");
    }
}

// ---------- 在线列表 ----------

function updateUserList(users) {
    userList.innerHTML = "";

    const others = users.filter((username) => username !== nickname);
    onlineCount.textContent = `${others.length} 人在线`;

    if (others.length === 0) {
        const empty = document.createElement("div");
        empty.className = "user-list-empty";
        empty.textContent = "暂无其他在线用户";
        userList.appendChild(empty);
    }

    others.forEach(function (username) {
        const user = document.createElement("div");
        user.className = "user";
        user.dataset.user = username;

        const name = document.createElement("span");
        name.className = "user-name";
        name.textContent = username;

        const badge = document.createElement("span");
        badge.className = "badge";

        user.appendChild(name);
        user.appendChild(badge);
        user.addEventListener("click", function () {
            openPrivateChat(username);
        });
        userList.appendChild(user);
    });

    renderBadges();
}

// ---------- WebSocket 连接与重连 ----------

function setConnectionStatus(text, state) {
    connectionStatus.textContent = text;
    connectionStatus.className = `conn-status conn-${state}`;
}

function connectWebSocket() {
    if (reconnectTimer) {
        clearTimeout(reconnectTimer);
        reconnectTimer = null;
    }

    setConnectionStatus("连接中…", "connecting");
    ws = new WebSocket(buildWsUrl());

    ws.onopen = function () {
        reconnectAttempt = 0;
        setConnectionStatus("已连接", "online");
        ws.send(JSON.stringify({ event: "lobby.enter" }));
        ws.send(JSON.stringify({ event: "lobby.history.pull" }));

        if (currentType === "private" && currentUser) {
            const privateSessionId = privateSessions[currentUser];
            if (privateSessionId) {
                ws.send(
                    JSON.stringify({
                        event: "private.history.pull",
                        payload: { privateSessionId },
                    })
                );
            }
        }
    };

    ws.onmessage = function (event) {
        try {
            handleWebSocketMessage(JSON.parse(event.data));
        } catch (error) {
            console.error("无法处理服务器消息：", error);
        }
    };

    ws.onclose = function () {
        setConnectionStatus("已断开", "offline");
        if (!manualClose) {
            reconnectAttempt += 1;
            const delay = Math.min(1000 * Math.pow(2, reconnectAttempt - 1), 10000);
            reconnectTimer = setTimeout(connectWebSocket, delay);
        }
    };

    ws.onerror = function () {
        // 关闭事件会随后触发并处理重连，这里仅记录
        console.error("WebSocket 连接错误");
    };
}

// ---------- 发送消息 ----------

function autoResizeInput() {
    input.style.height = "auto";
    input.style.height = Math.min(input.scrollHeight, 120) + "px";
}

async function sendCurrentMessage() {
    const content = input.value.trim();
    if (!content) {
        return;
    }
    if (!ws || ws.readyState !== WebSocket.OPEN) {
        showToast("聊天服务器尚未连接", "error");
        return;
    }

    if (currentType === "lobby") {
        ws.send(
            JSON.stringify({
                event: "lobby.message.send",
                payload: { content },
            })
        );
    } else if (currentType === "private" && currentUser) {
        const privateSessionId = privateSessions[currentUser];
        if (!privateSessionId) {
            showToast("私聊窗口尚未创建", "error");
            return;
        }
        ws.send(
            JSON.stringify({
                event: "private.message.send",
                payload: { privateSessionId, content },
            })
        );
    }

    input.value = "";
    autoResizeInput();
    input.focus();
}

// ---------- 事件绑定 ----------

lobbyButton.addEventListener("click", switchToLobby);

sendButton.addEventListener("click", sendCurrentMessage);

input.addEventListener("input", autoResizeInput);

input.addEventListener("keydown", function (event) {
    if (event.key === "Enter" && !event.shiftKey) {
        event.preventDefault();
        sendCurrentMessage();
    }
});

logoutButton.addEventListener("click", async function () {
    logoutButton.disabled = true;
    try {
        await apiFetch("/api/v1/auth/logout", {
            method: "POST",
            auth: true,
        });
        manualClose = true;
        if (ws) {
            ws.close();
        }
        clearSession();
        window.location.href = "login.html";
    } catch (error) {
        showToast(error.message || "退出登录失败", "error");
        logoutButton.disabled = false;
    }
});

// ---------- 初始化 ----------

showMessages();
connectWebSocket();