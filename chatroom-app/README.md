# 聊天室 · 多端前端（Web + 微信小程序）

基于 uni-app（Vue 3 + TypeScript + Vite）的聊天室前端，一套代码同时构建 **Web 端** 与 **微信小程序端**，连接同一套 C++ Crow + MySQL 后端。

- 小程序 AppID：`wxa47a07d61caf85c1`
- 本机后端：`C:\Users\31586\Desktop\网络聊天室\chatroom`，健康检查 `http://127.0.0.1:8080/api/v1/health`
- 项目位置：后端仓库根目录下的 `chatroom-app/`，后端整体说明见上级 `README.md`
- 当前状态：本机开发调试可用；尚未上传微信版本、提交审核或正式发布

## 功能范围

沿用原小程序全部业务能力，未改动接口协议与业务流程：

- 昵称 + 密码注册 / 登录 / 退出；账号单端在线，会话与服务器绑定
- 公共大厅消息、在线成员列表、历史消息拉取
- 一对一私聊、未读计数、私聊窗口失效自动重建
- 断线指数退避重连、切后台暂停 / 回前台恢复、会话失效跳转登录
- 消息去重、最近 200 条容量上限、1–1000 字输入校验
- 开发连接设置：保存并检测后端服务器地址（仅开发环境）

## 目录结构

```
src/
├── components/         公共组件库（按钮、输入、头像、气泡、侧栏、输入条…）
├── composables/        组合式函数（聊天状态桥接、视口宽度监听）
├── pages/
│   ├── login/          登录与注册
│   └── chat/           大厅 / 私聊
├── services/           配置、接口请求、会话存储、Socket 聊天客户端
├── styles/base.scss    全局基础样式与动效关键帧
├── uni.scss            设计令牌：颜色 / 字体 / 间距 / 圆角 / 阴影 / 断点 / mixin
├── App.vue
├── main.ts
├── manifest.json       多端构建配置（H5 / 微信小程序）
└── pages.json          页面注册与窗口样式
scripts/
├── check.cjs           结构、品牌残留与发布配置检查
├── test.cjs            真实后端端到端联调
└── sync-frontend.cjs   把 Web 构建产物同步到后端托管目录 ../frontend
```

## 本机运行

```powershell
npm install          # 首次安装依赖

# Web 端：http://localhost:5173 ，/api/v1 与 /ws/chat 由 Vite 代理到 127.0.0.1:8080
npm run dev:h5

# 微信小程序：产物在 dist/dev/mp-weixin ，用微信开发者工具导入该目录
npm run dev:mp-weixin
```

后端需先运行（`start-backend.ps1`）。小程序工程配置由 `src/manifest.json` 的 `mp-weixin` 段生成，其中已关闭 `urlCheck`，本机调试可直接访问 `127.0.0.1`。

手机预览时 `127.0.0.1` 指向手机自己，需要后端可被局域网访问，或在登录页「开发连接设置」中填写可访问的服务器地址。正式版小程序必须使用已配置为合法域名的 HTTPS/WSS 地址。

## 构建与发布

```powershell
npm run build:h5          # 产出 dist/build/h5 ，可直接部署为静态站点
npm run build:backend     # 构建 Web 端并同步到 ../frontend ，由后端直接托管
npm run build:mp-weixin   # 产出 dist/build/mp-weixin ，用开发者工具上传体验版/正式版
```

`npm run build:backend` 等价于 `build:h5` + `sync:frontend`。同步后 <http://localhost:8080> 与开发服务器 <http://localhost:5173> 是同一套页面与在线逻辑，刷新即可生效，无需重启后端（后端根路由固定返回的 `login.html` 会跳转到单页应用入口）。

Web 端三种连接方式，任选其一：

1. **同源部署（推荐）**：把 `dist/build/h5` 交给反向代理，`/api/v1` 与 `/ws/chat`（含 WebSocket Upgrade）转发到后端 `127.0.0.1:8080`，页面与接口同源，无 CORS 问题。
2. **独立域名**：把 `src/services/config.ts` 的 `RELEASE_API_BASE` 设置为 HTTPS 根地址（不带 `/api/v1`），后端需允许该来源跨域。
3. **开发期**：默认走 Vite 同源代理，无需任何配置。

小程序端发布前：

1. 设置 `src/services/config.ts` 的 `RELEASE_API_BASE` 为公网 HTTPS 根地址，并执行 `npm run check -- --release` 校验（未配置会失败，属预期保护）。
2. 在小程序后台配置 request 与 socket 合法域名。
3. 用开发者工具打开 `dist/build/mp-weixin` 上传代码，再按需设为体验版、提交审核。

**公网部署前还需加固原后端：** 当前账号实现直接比较密码，没有密码哈希；还应补上在线会话最长有效期、访问频率限制及聊天内容管理。

## 校验与测试

```powershell
npm run type-check   # TypeScript 类型检查
npm run check        # 工程结构、页面注册、品牌残留、发布配置检查
npm test             # 真实后端端到端联调（需先启动后端与 MySQL）
```

`npm test` 会创建独立的 `wx_test_` 前缀账号，覆盖注册、登录、重复昵称、错误密码、大厅双端收发、私聊历史与未读、自动重连、前后台恢复、退出后私聊清空与会话失效跳转；结束后自动退出登录，不修改已有账号。后端地址可用 `TEST_API_BASE` 覆盖。

## 多端与响应式

- 布局断点统一在 `src/uni.scss`：`900px` 起聊天页切换为「侧栏 + 会话」双栏，`768px` 以下为抽屉式侧栏；Web 桌面浏览器、平板与微信小程序（手机 / 平板 / Windows 微信窗口）共用同一套断点。
- 字号与组件尺寸统一使用 `px`，保证手机 / 平板 / 桌面端可读性一致；布局断点、留白与圆角在 `src/uni.scss` 统一维护。