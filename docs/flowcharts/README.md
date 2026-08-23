# 流程图目录说明

本目录用于存放浏览器聊天室后端的业务流程说明文档。当前版本已经和现有架构、主要功能、实现方式保持一致，可直接作为后续 Mermaid 流程图绘制依据。

## 目录内容

- `register_flow.md`：注册流程
- `login_flow.md`：登录流程
- `logout_flow.md`：登出与清理流程
- `ws_connect_flow.md`：WebSocket 建连与在线状态流程
- `lobby_chat_flow.md`：大厅聊天流程
- `lobby_history_flow.md`：大厅历史可见记录流程
- `private_session_flow.md`：创建私聊窗口会话流程
- `private_chat_flow.md`：私聊消息收发流程
- `private_history_flow.md`：当前私聊窗口历史流程
- `online_user_flow.md`：在线用户列表与在线变更通知流程

## 流程关系

1. 用户先完成 `register_flow.md` 或直接进入 `login_flow.md`
2. 登录成功后拿到 `sessionId`
3. 前端携带 `sessionId` 发起 `ws_connect_flow.md`
4. WebSocket 建连成功后，用户才会被记为在线用户
5. 用户在线后可以进入大厅、查看在线用户、创建私聊窗口、进行大厅聊天和私聊
6. 用户登出时执行 `logout_flow.md`，并清空和该用户有关的私聊窗口

## 关键约束

- 昵称唯一，系统中不再单独分配账号
- 登录成功后返回 `sessionId`
- 在线用户定义为“已登录且 WebSocket 已连接的用户”
- 大厅历史只显示用户进入大厅之后的消息
- 私聊历史只记录当前私聊窗口创建之后的消息
- 私聊窗口记录使用 `map` 管理
- 当前阶段不接数据库，流程全部基于内存存储设计

## 使用建议

- 成员1可基于本目录讲整体架构流程
- 成员2可重点讲 `ws_connect_flow.md` 和实时消息流转
- 成员3可重点讲注册、登录、登出流程
- 成员4可重点讲大厅聊天、私聊、在线用户流程
- 成员5可重点讲内存数据流与 `map` 管理策略
- 成员6可直接把本目录转换为 Mermaid 流程图和答辩材料
