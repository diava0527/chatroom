# 一键启动后端：读取本地 .env 配置并运行聊天室服务。
# 仓库里只提交 .env.example（占位值），真实的 .env 已被 .gitignore 忽略，不会进版本库。
# 用法：./start-backend.ps1           自动使用 out/build 下最新编译出的 exe
#       ./start-backend.ps1 -ExePath <路径>   指定其它 exe

param([string]$ExePath)

$ErrorActionPreference = 'Stop'
$projectDirectory = $PSScriptRoot

# 记录本脚本注入到当前进程的环境变量，退出时清理：
# 既避免密码留在当前窗口，也让同一窗口里修改 .env 后能立即生效
$injectedKeys = [System.Collections.Generic.HashSet[string]]::new()

# 1. 读取 .env（存在时生效）。已存在的环境变量优先级更高，便于 CI / 服务器覆盖。
$envFile = Join-Path $projectDirectory '.env'
if (Test-Path -LiteralPath $envFile) {
    foreach ($line in Get-Content -LiteralPath $envFile -Encoding utf8) {
        # TrimStart 去掉记事本可能写入的 BOM，避免第一行的 key 带上不可见字符
        $trimmed = $line.Trim().TrimStart([char]0xFEFF)
        if ($trimmed -eq '' -or $trimmed.StartsWith('#')) { continue }
        $separator = $trimmed.IndexOf('=')
        if ($separator -lt 1) { continue }

        $key = $trimmed.Substring(0, $separator).Trim()
        $value = $trimmed.Substring($separator + 1).Trim()
        if ($value.Length -ge 2 -and $value.StartsWith('"') -and $value.EndsWith('"')) {
            $value = $value.Substring(1, $value.Length - 2)
        }
        if ($key -notmatch '^[A-Za-z_][A-Za-z0-9_]*$') {
            Write-Warning "忽略无法识别的配置行：$trimmed"
            continue
        }
        if (!(Test-Path "Env:$key")) {
            [System.Environment]::SetEnvironmentVariable($key, $value, 'Process')
            $null = $injectedKeys.Add($key)
        }
    }
    Write-Host "已读取配置：$envFile"
} else {
    Write-Host "未找到 .env，使用默认值；可复制 .env.example 为 .env 后按需修改。"
}

# 2. 非敏感默认值（.env 或已有的环境变量没提供时才补）
$defaults = [ordered]@{
    DB_HOST                    = '127.0.0.1'
    DB_PORT                    = '3306'
    DB_NAME                    = 'chatroom'
    DB_USER                    = 'chat_app'
    DB_CONNECT_TIMEOUT_SECONDS = '5'
    FRONTEND_DIR               = 'frontend'
    FRONTEND_ORIGIN            = 'http://localhost:8080'
}
foreach ($name in $defaults.Keys) {
    if (![System.Environment]::GetEnvironmentVariable($name)) {
        [System.Environment]::SetEnvironmentVariable($name, $defaults[$name], 'Process')
        $null = $injectedKeys.Add($name)
    }
}

# 3. 找最新编译出的可执行文件（先确认程序存在，再问密码）
if ($ExePath) {
    if (!(Test-Path -LiteralPath $ExePath -PathType Leaf)) {
        throw "指定的可执行文件不存在：$ExePath"
    }
    # 先转成绝对路径，避免后面切换工作目录后相对路径失效
    $ExePath = (Resolve-Path -LiteralPath $ExePath).Path
} else {
    $buildRoot = Join-Path $projectDirectory 'out\build'
    if (!(Test-Path -LiteralPath $buildRoot)) {
        throw '还没有构建产物，请先执行 ./build.ps1。'
    }
    $candidate = Get-ChildItem -Path $buildRoot -Recurse `
        -Filter 'browser_chatroom_architecture.exe' -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if (!$candidate) {
        throw '没找到 browser_chatroom_architecture.exe，请先执行 ./build.ps1，或用 -ExePath 指定路径。'
    }
    $ExePath = $candidate.FullName
}

# 4. 密码只来自 .env 或本次输入，不写入任何文件
if (!$env:DB_PASSWORD -or $env:DB_PASSWORD -eq 'change_me') {
    $securePassword = Read-Host 'MySQL 密码（输入不显示，也不会写入文件）' -AsSecureString
    $env:DB_PASSWORD = [System.Net.NetworkCredential]::new('', $securePassword).Password
    $null = $injectedKeys.Add('DB_PASSWORD')
    Remove-Variable securePassword
}

Write-Host "后端程序：$ExePath"
Write-Host "前端目录：$env:FRONTEND_DIR（相对路径，以项目根目录为基准）"
Write-Host '启动后浏览器访问 http://localhost:8080/'

# 5. 必须以项目根目录为工作目录，静态前端才能按相对路径找到
Push-Location -LiteralPath $projectDirectory
try {
    & $ExePath
}
finally {
    Pop-Location
    # 清掉本脚本注入的变量：避免密码留在当前窗口，也保证下次运行重新读取 .env
    foreach ($name in $injectedKeys) {
        Remove-Item -Path "Env:$name" -ErrorAction SilentlyContinue
    }
}

# 把服务的退出码回传给调用方，方便脚本或 CI 判断是否启动失败
if ($null -ne $LASTEXITCODE) { exit $LASTEXITCODE }