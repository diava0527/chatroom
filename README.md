# 网络聊天室后端架构

本项目是基于 `crow_all.h` 与 Boost 的浏览器聊天室后端架构骨架，只保留项目结构、模块职责、接口定义和前后端联调格式，不实现具体业务逻辑代码。

## 主要功能

- 账号体系改为昵称体系，昵称唯一且不可重复
- 用户使用昵称和密码注册
- 用户使用昵称和密码登录
- 登录成功后返回 `sessionId`
- 用户进入聊天大厅后可以发送大厅消息
- 大厅历史只显示用户进入大厅之后的消息
- 用户可以查看在线用户并主动创建私聊窗口
- 私聊历史只记录从当前私聊窗口开始后的消息
- 用户退出登录后，清空和该用户有关的私聊窗口

## 实现方式

- 后端通信采用 `HTTP + WebSocket` 混合架构
- 注册、登录、登出、创建私聊窗口、获取在线用户使用 HTTP
- 大厅聊天、私聊消息、历史拉取、在线状态通知使用 WebSocket
- 私聊记录使用 `map` 管理当前私聊窗口会话
- 已加入 MySQL 连接层；当前业务 Store 仍使用内存实现，后续逐个替换为 MySQL Store

## MySQL 本地接入

项目通过 MySQL 官方 C API 连接数据库，连接参数只从环境变量读取，不把数据库密码写进源码。

1. 使用管理员 PowerShell 启动服务：

   ```powershell
   Start-Service MySQL80
   ```

2. 使用 MySQL 管理员账号创建项目数据库和最小权限账号（请替换示例密码）：

   ```sql
   CREATE DATABASE IF NOT EXISTS chatroom
     CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci;
   CREATE USER IF NOT EXISTS 'chat_app'@'127.0.0.1'
     IDENTIFIED BY 'replace_with_a_strong_password';
   GRANT ALL PRIVILEGES ON chatroom.* TO 'chat_app'@'127.0.0.1';
   ```

3. 在运行程序的 PowerShell 会话中设置连接参数：

   ```powershell
   $env:DB_HOST = '127.0.0.1'
   $env:DB_PORT = '3306'
   $env:DB_NAME = 'chatroom'
   $env:DB_USER = 'chat_app'
   $securePassword = Read-Host 'MySQL password' -AsSecureString
   $env:DB_PASSWORD = [System.Net.NetworkCredential]::new('', $securePassword).Password
   Remove-Variable securePassword
   $env:DB_CONNECT_TIMEOUT_SECONDS = '5'
   ```

4. 编译并运行独立连接检查：

   ```powershell
   cmake -S . -B out/build/mysql-vs -G "Visual Studio 17 2022" -A x64
   cmake --build out/build/mysql-vs --config RelWithDebInfo --target mysql_connection_check
   .\out\build\mysql-vs\RelWithDebInfo\mysql_connection_check.exe
   ```

连接成功后会输出 MySQL 服务版本、主机、数据库和账号，但不会输出密码。可用变量清单见 `.env.example`。

## 架构要求

- 不实现具体业务逻辑代码
- 每个核心接口保留中文注释
- 注释格式统一为：
  - `1)代码逻辑：`
  - `2)返回值类型 类型原因 (哪个接口调用)`
  - `3)参数类型 类型原因 (为什么是这个参数，这个参数符合代码逻辑的哪个要求)`

## 当前目录

- `include/framework/`：Crow 启动装配、路由注册、统一响应格式
- `include/models/`：用户、消息、大厅会话、私聊窗口会话模型
- `include/websocket/`：WebSocket 连接管理、消息分发、聊天入口控制
- `include/user/`：注册、登录、登出、鉴权接口
- `include/chat/`：大厅、私聊、在线用户、聊天协议接口
- `include/storage/`：内存用户、会话、大厅消息、私聊窗口存储
- `include/common/`：公共类型和常量
- `src/`：程序启动入口
# chatroom

网络聊天室

- arch:项目架构
- doc:项目相关文档
