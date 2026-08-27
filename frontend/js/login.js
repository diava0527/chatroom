const usernameInput = document.getElementById("username");
const passwordInput = document.getElementById("password");
const loginButton = document.getElementById("loginButton");

loginButton.addEventListener("click", async function () {

    const nickname = usernameInput.value.trim();
    const password = passwordInput.value;

    // 检查输入
    if (nickname === "" || password === "") {
        alert("请输入用户名和密码");
        return;
    }

    try {

        const response = await fetch(
            "http://localhost:8080/api/v1/auth/login",
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

        console.log("登录结果：", result);

        // 登录成功
        if (result.code === 0) {

            // 保存昵称
            localStorage.setItem(
                "nickname",
                result.data.nickname
            );

            // 保存 sessionId
            localStorage.setItem(
                "sessionId",
                result.data.sessionId
            );

            alert("登录成功");

            // 进入聊天室
            window.location.href = "index.html";

        } else {

            alert(result.message);
        }

    } catch (error) {

        console.error("登录失败：", error);
        alert("无法连接服务器");

    }

});