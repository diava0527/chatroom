const usernameInput = document.getElementById("username");
const passwordInput = document.getElementById("password");
const loginButton = document.getElementById("loginButton");

async function submitLogin() {
    const nickname = usernameInput.value.trim();
    const password = passwordInput.value;

    if (!nickname || !password) {
        showToast("请输入用户名和密码", "error");
        return;
    }

    loginButton.disabled = true;
    loginButton.textContent = "登录中…";

    try {
        const result = await apiFetch("/api/v1/auth/login", {
            method: "POST",
            body: { nickname, password },
        });

        saveSession(result.data.nickname, result.data.sessionId);
        showToast("登录成功", "success");
        setTimeout(() => {
            window.location.href = "index.html";
        }, 300);
    } catch (error) {
        showToast(error.message || "无法连接服务器", "error");
    } finally {
        loginButton.disabled = false;
        loginButton.textContent = "登录";
    }
}

loginButton.addEventListener("click", submitLogin);

[usernameInput, passwordInput].forEach((input) => {
    input.addEventListener("keydown", (event) => {
        if (event.key === "Enter") {
            event.preventDefault();
            submitLogin();
        }
    });
});

// 自动聚焦用户名输入框
usernameInput.focus();