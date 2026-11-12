# 一键启动后端：读取本地 .env 配置并运行聊天室服务。
# 仓库里只提交 .env.example（占位值），真实的 .env 已被 .gitignore 忽略，不会进版本库。
# 用法：./start-backend.ps1           自动使用 out/build 下最新编译出的 exe
#       ./start-backend.ps1 -ExePath <路径>   指定其它 exe

param([string]$ExePath)

$ErrorActionPreference = 'Stop'
$projectDirectory = $PSScriptRoot

# 1. 读取 .env（存在时生效）。已存在的环境变量优先级更高，便于 CI / 服务器覆盖。
$envFile = Join-Path $projectDirectory '.env'
if (Test-Path -LiteralPath $envFile) {
    foreach ($line in Get-Content -LiteralPath $envFile) {
        $trimmed = $line.Trim()
        if ($trimmed -eq '' -or $trimmed.StartsWith('#')) { continue }
        $separator = $trimmed.IndexOf('=')
        if ($separator -lt 1) { continue }

        $key = $trimmed.Substring(0, $separator).Trim()
        $value = $trimmed.Substring($separator + 1).Trim()
        if ($value.Length -ge 2 -and $value.StartsWith('"') -and $value.EndsWith('"')) {
            $value = $value.Substring(1, $value.Length - 2)
        }
        if (!(Test-Path "Env:$key")) {
            [System.Environment]::SetEnvironmentVariable($key, $value, 'Process')
        }
    }
    Write-Host "已读取配置：$envFile"
} else {
    Write-Host "未找到 .env，使用默认值；可复制 .env.example 为 .env 后按需修改。"
}

# 2. 非敏感默认值
if (!$env:DB_HOST) { $env:DB_HOST = '127.0.0.1' }
if (!$env:DB_PORT) { $env:DB_PORT = '3306' }
if (!$env:DB_NAME) { $env:DB_NAME = 'chatroom' }
if (!$env:DB_USER) { $env:DB_USER = 'chat_app' }
if (!$env:DB_CONNECT_TIMEOUT_SECONDS) { $env:DB_CONNECT_TIMEOUT_SECONDS = '5' }
if (!$env:FRONTEND_DIR) { $env:FRONTEND_DIR = 'frontend' }
if (!$env:FRONTEND_ORIGIN) { $env:FRONTEND_ORIGIN = 'http://localhost:8080' }

# 3. 密码只来自 .env 或本次输入，不写入任何文件
if (!$env:DB_PASSWORD -or $env:DB_PASSWORD -eq 'change_me') {
    $securePassword = Read-Host 'MySQL 密码（输入不显示，也不会写入文件）' -AsSecureString
    $env:DB_PASSWORD = [System.Net.NetworkCredential]::new('', $securePassword).Password
    Remove-Variable securePassword
}

# 4. 找最新编译出的可执行文件
if (!$ExePath) {
    $candidate = Get-ChildItem -Path (Join-Path $projectDirectory 'out\build') `
        -Recurse -Filter 'browser_chatroom_architecture.exe' -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if (!$candidate) {
        throw '没找到 browser_chatroom_architecture.exe，请先编译，或用 -ExePath 指定路径。'
    }
    $ExePath = $candidate.FullName
}

Write-Host "后端程序：$ExePath"
Write-Host "前端目录：$env:FRONTEND_DIR（相对路径，以项目根目录为基准）"
Write-Host '启动后浏览器访问 http://localhost:8080/'

# 5. 必须以项目根目录为工作目录，静态前端才能按相对路径找到
Push-Location -LiteralPath $projectDirectory
try {
    & $ExePath
} finally {
    Pop-Location
}