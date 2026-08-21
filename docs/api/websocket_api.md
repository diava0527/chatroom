# WebSocket API 联调格式

## 1. 建立连接

- 地址：`ws://<host>:<port>/ws/chat?sessionId=session_xxx`
- 连接成功后，后端根据 `sessionId` 识别登录用户

## 2. 客户端发送事件总格式

```json
{
  "event": "event_name",
  "payload": {}
}
```

## 3. 进入大厅

客户端发送：

```json
{
  "event": "lobby.enter",
  "payload": {}
}
```

服务端响应：

```json
{
  "event": "lobby.enter.ack",
  "payload": {
    "nickname": "alice",
    "enteredAt": "2026-08-21 22:00:00"
  }
}
```

## 4. 发送大厅消息

客户端发送：

```json
{
  "event": "lobby.message.send",
  "payload": {
    "content": "hello everyone"
  }
}
```

服务端广播：

```json
{
  "event": "lobby.message.receive",
  "payload": {
    "messageId": "msg_xxx",
    "senderNickname": "alice",
    "receiverNickname": "LOBBY",
    "content": "hello everyone",
    "timestamp": "2026-08-21 22:01:00"
  }
}
```

## 5. 获取当前大厅可见历史

客户端发送：

```json
{
  "event": "lobby.history.pull",
  "payload": {}
}
```

服务端响应：

```json
{
  "event": "lobby.history.response",
  "payload": {
    "messages": [
      {
        "messageId": "msg_001",
        "senderNickname": "alice",
        "receiverNickname": "LOBBY",
        "content": "hello",
        "timestamp": "2026-08-21 22:01:00"
      }
    ]
  }
}
```

## 6. 发送私聊消息

客户端发送：

```json
{
  "event": "private.message.send",
  "payload": {
    "privateSessionId": "private_session_xxx",
    "content": "hi bob"
  }
}
```

服务端推送：

```json
{
  "event": "private.message.receive",
  "payload": {
    "privateSessionId": "private_session_xxx",
    "message": {
      "messageId": "msg_xxx",
      "senderNickname": "alice",
      "receiverNickname": "bob",
      "content": "hi bob",
      "timestamp": "2026-08-21 22:03:00"
    }
  }
}
```

## 7. 获取当前私聊窗口会话历史

客户端发送：

```json
{
  "event": "private.history.pull",
  "payload": {
    "privateSessionId": "private_session_xxx"
  }
}
```

服务端响应：

```json
{
  "event": "private.history.response",
  "payload": {
    "privateSessionId": "private_session_xxx",
    "messages": [
      {
        "messageId": "msg_xxx",
        "senderNickname": "alice",
        "receiverNickname": "bob",
        "content": "hi bob",
        "timestamp": "2026-08-21 22:03:00"
      }
    ]
  }
}
```

## 8. 在线用户变化通知

服务端推送：

```json
{
  "event": "online.users.changed",
  "payload": {
    "onlineUsers": [
      "alice",
      "bob"
    ]
  }
}
```
