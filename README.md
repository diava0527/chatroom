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
- 用户、大厅消息、私聊窗口和私聊消息使用 MySQL Store
- 登录 session、在线状态和 WebSocket 连接保留在进程内存中

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

程序首次启动时会在 `chatroom` 数据库中自动创建业务表，因此 `chat_app` 需要该数据库的建表和读写权限。

## 启动聊天室

1. 在已设置数据库环境变量的 PowerShell 中编译并启动后端：

   ```powershell
   cmake -S . -B out/build/mysql-vs -G "Visual Studio 17 2022" -A x64
   cmake --build out/build/mysql-vs --config RelWithDebInfo --target browser_chatroom_architecture
   .\out\build\mysql-vs\RelWithDebInfo\browser_chatroom_architecture.exe
   ```

2. 新开一个 PowerShell，在项目根目录启动前端静态服务器：

   ```powershell
   python -m http.server 5500 --directory frontend
   ```

3. 浏览器访问 `http://localhost:5500/login.html`。后端健康检查地址为 `http://127.0.0.1:8080/api/v1/health`。

后端默认只允许 `http://localhost:5500` 跨域访问。需要使用其他前端地址时，在启动后端前设置 `FRONTEND_ORIGIN`。

## 关闭网页与重新登录

关闭网页后，WebSocket 断连会释放昵称的在线登录占用，用户可以立即用正确密码重新登录；新登录会撤销旧 session，并清理旧私聊窗口。仍有活动连接的账号继续拒绝重复登录。

为避免刷新、短暂断网或小程序切后台导致误退出，断连会话保留 5 分钟重连宽限期。期间原 session 可以恢复，HTTP 查询不会延长宽限期。超时后凭据失效，服务器每 15 秒回收一次并清理私聊。登录后一直未建立 WebSocket 的会话在 15 秒后失效。旧连接迟到的关闭回调不能释放新连接的登录占用。

网页前端在页面离开时关闭连接，不依赖可能丢失的 unload 退出请求；重连前验证 session，失效则返回登录页，避免无限重连。

回归检查：

```powershell
cmake -S . -B out/build/session-fix -G "Visual Studio 17 2022" -A x64
cmake --build out/build/session-fix --config RelWithDebInfo --target session_lifecycle_test session_socket_server browser_chatroom_architecture mysql_connection_check
ctest --test-dir out/build/session-fix -C RelWithDebInfo --output-on-failure
node --check frontend/js/main.js
```

运行中的旧后端不会自动加载新代码。在原启动后端的 PowerShell 中执行 `./apply-backend-fix.ps1` 即可先检查数据库连接，再备份旧程序、更新原运行路径并重启；没有数据库环境变量时会安全提示输入密码，不会写入源码或文件。更新失败会还原旧程序。重启会清空内存中的登录状态，但不会删除账号数据。

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
