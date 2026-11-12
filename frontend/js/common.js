// ================================
// 前端公共模块
// 统一后端地址、请求封装、会话读写与轻提示
// ================================

const ChatConfig = {
    // 页面由后端同一端口托管，接口和页面同源，用空串走相对路径即可
    API_BASE: "",
    SESSION_KEY: "sessionId",
    NICKNAME_KEY: "nickname",
    WS_PATH: "/ws/chat",
};

// ---------- 会话读写 ----------

function getSessionId() {
    return localStorage.getItem(ChatConfig.SESSION_KEY);
}

function getNickname() {
    return localStorage.getItem(ChatConfig.NICKNAME_KEY);
}

function saveSession(nickname, sessionId) {
    localStorage.setItem(ChatConfig.NICKNAME_KEY, nickname);
    localStorage.setItem(ChatConfig.SESSION_KEY, sessionId);
}

function clearSession() {
    localStorage.removeItem(ChatConfig.SESSION_KEY);
    localStorage.removeItem(ChatConfig.NICKNAME_KEY);
}

function buildWsUrl() {
    // WebSocket 地址跟随当前页面来源：http 用 ws，https 用 wss
    const protocol = location.protocol === "https:" ? "wss:" : "ws:";
    const sid = getSessionId() || "";
    return `${protocol}//${location.host}${ChatConfig.WS_PATH}?sessionId=${encodeURIComponent(sid)}`;
}

// ---------- HTTP 请求封装 ----------

// 统一请求：自动加 JSON 头，可选携带 X-Session-Id，
// 后端 code !== 0 或 HTTP 非 2xx 时抛错（err.message 为后端 message）。
async function apiFetch(path, options = {}) {
    const headers = { "Content-Type": "application/json" };
    if (options.auth) {
        headers["X-Session-Id"] = getSessionId() || "";
    }

    const response = await fetch(ChatConfig.API_BASE + path, {
        method: options.method || "GET",
        headers,
        body: options.body ? JSON.stringify(options.body) : undefined,
    });

    let result;
    try {
        result = await response.json();
    } catch (error) {
        const err = new Error("服务器返回异常");
        err.status = response.status;
        throw err;
    }

    if (!response.ok || result.code !== 0) {
        const err = new Error(result.message || "请求失败");
        err.code = result.code;
        throw err;
    }
    return result;
}

// ---------- 轻提示 toast ----------

function showToast(message, type = "info") {
    let box = document.querySelector(".toast-box");
    if (!box) {
        box = document.createElement("div");
        box.className = "toast-box";
        document.body.appendChild(box);
    }

    const toast = document.createElement("div");
    toast.className = `toast toast-${type}`;
    toast.textContent = message;
    box.appendChild(toast);

    setTimeout(() => {
        toast.classList.add("toast-out");
        setTimeout(() => toast.remove(), 300);
    }, 2200);
}