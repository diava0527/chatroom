const sessionId = localStorage.getItem("sessionId");
const nickname = localStorage.getItem("nickname");

console.log("当前用户：", nickname);
console.log("sessionId：", sessionId);

// WebSocket 地址
// 后端确定地址以后，只需要修改这里
const WS_URL = "ws://127.0.0.1:8080";

// 创建 WebSocket
let ws = null;

function connectWebSocket() {

    if (!sessionId) {
        console.log("没有 sessionId，暂时无法连接 WebSocket");
        return;
    }

    ws = new WebSocket(WS_URL);

    ws.onopen = function () {

        console.log("WebSocket 连接成功");

        // 进入公共大厅
        ws.send(JSON.stringify({
            event: "lobby.enter"
        }));

        // 获取大厅历史消息
        ws.send(JSON.stringify({
            event: "lobby.history.pull"
        }));
    };

    ws.onmessage = function (event) {

        const data = JSON.parse(event.data);

        console.log("收到后端消息：", data);

        handleMessage(data);
    };

    ws.onclose = function () {

        console.log("WebSocket 连接关闭");
    };

    ws.onerror = function (error) {

        console.error("WebSocket 错误：", error);
    };
}

function handleMessage(data) {

    // 进入大厅成功
    if (data.event === "lobby.enter.ack") {

        console.log("进入大厅成功");

        return;
    }


    // 收到大厅消息
    if (data.event === "lobby.message.receive") {

        const message = data.payload;

        chatHistory.lobby.push({
            senderNickname: message.senderNickname,
            content: message.content,
            timestamp: message.timestamp
        });

        // 如果当前正在看公共大厅，就刷新
        if (currentType === "lobby") {
            showMessages();
        }

        return;
    }


    // 收到大厅历史消息
    if (data.event === "lobby.history.response") {

        chatHistory.lobby = [];

        const messages = data.payload.messages || [];

        messages.forEach(function (message) {

            chatHistory.lobby.push({
                senderNickname: message.senderNickname,
                content: message.content,
                timestamp: message.timestamp
            });

        });

        if (currentType === "lobby") {
            showMessages();
        }

        return;
    }

    // 在线用户列表发生变化
        if (data.event === "online.users.changed") {

            const users = data.payload.users || [];

            updateUserList(users);

            return;
        }

    // 其他消息
    console.log("未处理的消息类型：", data.event);
}

function updateUserList(users) {

    // 清空旧用户
    userList.innerHTML = "";

    users.forEach(function (username) {

        // 不显示自己
        if (username === nickname) {
            return;
        }

        addUser(username);

    });

}

const userList = document.getElementById("userList");

const chatTitle = document.getElementById("chatTitle");
const chatBox = document.getElementById("chatBox");

const input = document.getElementById("messageInput");
const button = document.getElementById("sendButton");

// 当前聊天类型
// lobby = 公共大厅
// private = 私聊
let currentType = null;

// 当前私聊用户
let currentUser = null;

// 保存聊天记录
const chatHistory = {
    lobby: []
};


// ====================
// 公共大厅
// ====================

const lobby = document.querySelector(".lobby");

lobby.addEventListener("click", function () {

    currentType = "lobby";
    currentUser = null;

    chatTitle.textContent = "公共大厅";

    showMessages();

});


// ====================
// 点击用户
// ====================

function addUser(username) {

    const user = document.createElement("div");

    user.className = "user";

    user.dataset.user = username;

    user.textContent = "🟢 " + username;

    userList.appendChild(user);

    user.addEventListener("click", function () {

        currentType = "private";
        currentUser = username;

        chatTitle.textContent = "和 " + username + " 聊天";

        // 如果这个用户还没有聊天记录
        if (!chatHistory[username]) {
            chatHistory[username] = [];
        }

        showMessages();

    });

}


// ====================
// 显示聊天记录
// ====================

function showMessages() {

    chatBox.innerHTML = "";

    let history;

    if (currentType === "lobby") {

        history = chatHistory.lobby;

    } else if (currentType === "private") {

        history = chatHistory[currentUser];

    } else {

        return;
    }

    history.forEach(function (message) {

        // 消息外层
        const messageWrapper = document.createElement("div");
        messageWrapper.className = "message-wrapper";

        // 判断是不是自己发的
        if (message.senderNickname === nickname) {
            messageWrapper.classList.add("message-self");
        } else {
            messageWrapper.classList.add("message-other");
        }

        // 昵称
        const sender = document.createElement("div");
        sender.className = "message-sender";
        sender.textContent = message.senderNickname;

        // 气泡
        const messageElement = document.createElement("div");
        messageElement.className = "message-bubble";
        messageElement.textContent = message.content;

        const time = document.createElement("div");
        time.className = "message-time";
        time.textContent = message.timestamp;

        // 组装
        messageWrapper.appendChild(sender);
        messageWrapper.appendChild(messageElement);
        messageWrapper.appendChild(time);

        chatBox.appendChild(messageWrapper);

    });

    // 自动滚动到底部
    chatBox.scrollTop = chatBox.scrollHeight;
}


// ====================
// 点击发送
// ====================

button.addEventListener("click", function () {

    if (currentType === null) {

        alert("请先选择公共大厅或者一个用户");

        return;
    }

    const message = input.value.trim();

    if (message === "") {

        return;
    }


    // 公共大厅
    if (currentType === "lobby") {

        if (!ws || ws.readyState !== WebSocket.OPEN) {

            alert("聊天服务器还没有连接");

            return;
        }

        ws.send(JSON.stringify({
            event: "lobby.message.send",
            payload: {
                content: message
            }
        }));

    }


    // 私聊
    else if (currentType === "private") {

        if (!ws || ws.readyState !== WebSocket.OPEN) {

            alert("聊天服务器还没有连接");

            return;
        }

        // 暂时假设 privateSessionId 就是后端给你的私聊会话ID
        const privateSessionId = chatHistory[currentUser].privateSessionId;

        if (!privateSessionId) {

            alert("还没有私聊会话");

            return;
        }

        ws.send(JSON.stringify({
            event: "private.message.send",
            payload: {
                privateSessionId: privateSessionId,
                content: message
            }
        }));

    }


    input.value = "";

    showMessages();

});


// ====================
// Enter 发送
// Shift + Enter 换行
// ====================

input.addEventListener("keydown", function (event) {

    if (event.key === "Enter" && !event.shiftKey) {

        event.preventDefault();

        button.click();

    }

});

connectWebSocket();