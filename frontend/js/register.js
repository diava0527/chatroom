const usernameInput = document.getElementById("username");
const passwordInput = document.getElementById("password");
const confirmPasswordInput = document.getElementById("confirmPassword");
const registerButton = document.getElementById("registerButton");

async function submitRegister() {
    const nickname = usernameInput.value.trim();
    const password = passwordInput.value;
    const confirmPassword = confirmPasswordInput.value;

    if (!nickname || !password || !confirmPassword) {
        showToast("请填写完整信息", "error");
        return;
    }

    if (password !== confirmPassword) {
        showToast("两次密码不一致", "error");
        return;
    }

    registerButton.disabled = true;
    registerButton.textContent = "注册中…";

    try {
        await apiFetch("/api/v1/auth/register", {
            method: "POST",
            body: { nickname, password },
        });

        showToast("注册成功", "success");
        setTimeout(() => {
            window.location.href = "login.html";
        }, 300);
    } catch (error) {
        showToast(error.message || "无法连接服务器", "error");
    } finally {
        registerButton.disabled = false;
        registerButton.textContent = "注册";
    }
}

registerButton.addEventListener("click", submitRegister);

[usernameInput, passwordInput, confirmPasswordInput].forEach((input) => {
    input.addEventListener("keydown", (event) => {
        if (event.key === "Enter") {
            event.preventDefault();
            submitRegister();
        }
    });
});

usernameInput.focus();