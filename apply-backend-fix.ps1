# 在原启动后端的 PowerShell 执行。先验证数据库连接，再备份、替换并重启后端。
param([switch]$NoPrompt)
$ErrorActionPreference = 'Stop'
$projectDirectory = $PSScriptRoot
$stagedDirectory = Join-Path $projectDirectory 'out\build\session-fix\RelWithDebInfo'
$runtimeDirectory = Join-Path $projectDirectory 'out\build\mysql-vs\RelWithDebInfo'
$stagedExe = Join-Path $stagedDirectory 'browser_chatroom_architecture.exe'
$runtimeExe = Join-Path $runtimeDirectory 'browser_chatroom_architecture.exe'
$connectionCheck = Join-Path $stagedDirectory 'mysql_connection_check.exe'
foreach ($requiredFile in @($stagedExe, $runtimeExe, $connectionCheck)) {
    if (!(Test-Path -LiteralPath $requiredFile)) { throw "缺少已编译文件：$requiredFile" }
}

if (!$env:DB_HOST) { $env:DB_HOST = '127.0.0.1' }
if (!$env:DB_PORT) { $env:DB_PORT = '3306' }
if (!$env:DB_NAME) { $env:DB_NAME = 'chatroom' }
if (!$env:DB_USER) { $env:DB_USER = 'chat_app' }
if (!$env:DB_CONNECT_TIMEOUT_SECONDS) { $env:DB_CONNECT_TIMEOUT_SECONDS = '5' }
if (!$env:FRONTEND_ORIGIN) { $env:FRONTEND_ORIGIN = 'http://localhost:5500' }
$temporaryPassword = $false
if (!$env:DB_PASSWORD -or $env:DB_PASSWORD -in @('你的密码', 'change_me')) {
    if ($NoPrompt) { throw '当前终端没有数据库密码；未停止或修改运行中的后端。请在原启动终端执行本脚本。' }
    $securePassword = Read-Host '请输入 MySQL chat_app 密码（输入不会显示，也不会写入文件）' -AsSecureString
    $env:DB_PASSWORD = [System.Net.NetworkCredential]::new('', $securePassword).Password
    Remove-Variable securePassword
    $temporaryPassword = $true
}

try {
    & $connectionCheck
    if ($LASTEXITCODE -ne 0) { throw '数据库预检查失败，原服务保持不变。' }

    $originalProcesses = @(Get-Process | Where-Object {
        $_.ProcessName -like '*chatroom*' -and $_.Path -eq $runtimeExe
    })
    $unexpected = @(Get-Process | Where-Object {
        $_.ProcessName -like '*chatroom*' -and $_.Path -ne $runtimeExe
    })
    if ($unexpected.Count -gt 0) { throw '发现其他目录运行的聊天室后端。为避免误停，请先手动确认并关闭它。' }

    $backupDirectory = Join-Path $projectDirectory ('out\backups\backend-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
    New-Item -ItemType Directory -Path $backupDirectory | Out-Null
    Copy-Item -LiteralPath $runtimeExe -Destination $backupDirectory
    $runtimePdb = Join-Path $runtimeDirectory 'browser_chatroom_architecture.pdb'
    if (Test-Path -LiteralPath $runtimePdb) { Copy-Item -LiteralPath $runtimePdb -Destination $backupDirectory }

    Write-Host '数据库检查通过。即将重启聊天室，当前所有登录会话会失效，账号不会删除。'
    foreach ($process in $originalProcesses) {
        Stop-Process -Id $process.Id
        if (!$process.WaitForExit(5000)) { throw '旧后端未退出，未替换程序。' }
    }

    $newProcess = $null
    try {
        Copy-Item -LiteralPath $stagedExe -Destination $runtimeExe -Force
        Copy-Item -LiteralPath (Join-Path $stagedDirectory 'browser_chatroom_architecture.pdb') -Destination $runtimePdb -Force
        $newProcess = Start-Process -FilePath $runtimeExe -WorkingDirectory $projectDirectory -WindowStyle Hidden -PassThru `
            -RedirectStandardOutput (Join-Path $backupDirectory 'new-backend.stdout.log') `
            -RedirectStandardError (Join-Path $backupDirectory 'new-backend.stderr.log')
        $healthy = $false
        for ($attempt = 0; $attempt -lt 20; $attempt++) {
            Start-Sleep -Milliseconds 250
            if ($newProcess.HasExited) { break }
            try {
                $health = Invoke-RestMethod 'http://127.0.0.1:8080/api/v1/health' -TimeoutSec 2
                if ($health.code -eq 0 -and !$newProcess.HasExited) { $healthy = $true; break }
            } catch { }
        }
        if (!$healthy) { throw '新后端健康检查失败。' }
    } catch {
        $startupFailure = $_
        if ($newProcess -and !$newProcess.HasExited) {
            Stop-Process -Id $newProcess.Id
            $null = $newProcess.WaitForExit(5000)
        }
        Copy-Item -LiteralPath (Join-Path $backupDirectory 'browser_chatroom_architecture.exe') -Destination $runtimeExe -Force
        $backupPdb = Join-Path $backupDirectory 'browser_chatroom_architecture.pdb'
        if (Test-Path -LiteralPath $backupPdb) { Copy-Item -LiteralPath $backupPdb -Destination $runtimePdb -Force }
        if ($originalProcesses.Count -gt 0) {
            Start-Process -FilePath $runtimeExe -WorkingDirectory $projectDirectory -WindowStyle Hidden
        }
        throw "更新失败，已还原旧程序：$startupFailure"
    }
    Write-Host '修复版后端已运行。现在可使用 n 的原密码登录；关闭网页后也可重新登录。'
    Write-Host "旧程序备份：$backupDirectory"
} finally {
    if ($temporaryPassword) { Remove-Item Env:DB_PASSWORD }
}
