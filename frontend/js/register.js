const usernameInput = document.getElementById("username");
const passwordInput = document.getElementById("password");
const confirmPasswordInput = document.getElementById("confirmPassword");
const registerButton = document.getElementById("registerButton");

registerButton.addEventListener("click", async function () {

    const nickname = usernameInput.value.trim();
    const password = passwordInput.value;
    const confirmPassword = confirmPasswordInput.value;

    // 检查用户名和密码是否为空
    if (nickname === "" || password === "" || confirmPassword === "") {
        alert("请填写完整信息");
        return;
    }

    // 检查两次密码是否一致
    if (password !== confirmPassword) {
        alert("两次密码不一致");
        return;
    }

    try {

        const response = await fetch(
            "http://localhost:8080/api/v1/auth/register",
            {
                method: "POST",

                headers: {
                    "Content-Type": "application/json"
                },

                body: JSON.stringify({
                    nickname: nickname,
                    password: password
                })
            }
        );

        const result = await response.json();

        console.log("注册结果：", result);

        if (result.code === 0) {

            alert("注册成功");

            // 注册成功后回到登录页面
            window.location.href = "login.html";

        } else {

            alert(result.message);
        }

    } catch (error) {

        console.error("注册失败：", error);
        alert("无法连接服务器");

    }

});