# HTTP API 联调格式

## 1. 用户注册

- 方法：`POST`
- 路径：`/api/v1/auth/register`
- 请求头：`Content-Type: application/json`

请求体：

```json
{
  "nickname": "alice",
  "password": "123456"
}
```

成功响应：

```json
{
  "code": 0,
  "message": "register success",
  "data": {
    "nickname": "alice"
  }
}
```

失败响应：

```json
{
  "code": 1001,
  "message": "nickname already exists",
  "data": null
}
```

参数有误时：
```json
{
  "code": 1004,
  "message": "invalid request",
  "data": null
}
```

## 2. 用户登录

- 方法：`POST`
- 路径：`/api/v1/auth/login`
- 请求头：`Content-Type: application/json`

请求体：

```json
{
  "nickname": "alice",
  "password": "123456"
}
```

成功响应：

```json
{
  "code": 0,
  "message": "login success",
  "data": {
    "nickname": "alice",
    "sessionId": "session_xxx"
  }
}
```

失败响应：

```json
{
  "code": 1002,
  "message": "nickname or password error",
  "data": null
}
```

## 3. 用户登出

- 方法：`POST`
- 路径：`/api/v1/auth/logout`
- 请求头：
  - `Content-Type: application/json`
  - `X-Session-Id: session_xxx`

请求体：

```json
{}
```

成功响应：

```json
{
  "code": 0,
  "message": "logout success",
  "data": null
}
```

失败响应：
```json
{
  "code": 1003,
  "message": "sessionId invalid",
  "data": null
}
```

## 4. 创建私聊窗口会话

- 方法：`POST`
- 路径：`/api/v1/private-chat/session`
- 请求头：
  - `Content-Type: application/json`
  - `X-Session-Id: session_xxx`

请求体：

```json
{
  "targetNickname": "bob"
}
```

成功响应：

```json
{
  "code": 0,
  "message": "private session created",
  "data": {
    "privateSessionId": "private_session_xxx",
    "senderNickname": "alice",
    "receiverNickname": "bob"
  }
}
```

## 5. 获取在线用户列表

- 方法：`GET`
- 路径：`/api/v1/users/online`
- 请求头：
  - `X-Session-Id: session_xxx`

成功响应：

```json
{
  "code": 0,
  "message": "success",
  "data": {
    "onlineUsers": [
      "alice",
      "bob",
      "charlie"
    ]
  }
}
```
