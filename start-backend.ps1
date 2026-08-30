# 一键启动后端：设置环境变量并运行浏览器聊天室后端服务
# 使用前：把下面的 DB_PASSWORD 改成你的 chat_app 密码（仅需改这一次）

$env:DB_HOST = '127.0.0.1'
$env:DB_PORT = '3306'
$env:DB_NAME = 'chatroom'
$env:DB_USER = 'chat_app'
$env:DB_PASSWORD = '你的密码'
$env:DB_CONNECT_TIMEOUT_SECONDS = '5'
$env:FRONTEND_ORIGIN = 'http://localhost:5500'

Write-Host "Starting chatroom backend..."
& '.\out\build\mysql-vs\RelWithDebInfo\browser_chatroom_architecture.exe'
