# 首次初始化：配置 CMake 并编译出后端可执行文件。
# 注意：本脚本只负责“把源码变成 exe”，不会启动服务、不连数据库、不监听端口。
#       所有启动都统一交给 start-backend.ps1。
# 用法：
#   首次使用：./build.ps1
#   改了代码后：./build.ps1                （增量编译，很快）
#   改了 CMakeLists 或换编译器：./build.ps1 -Clean
#   需要跑测试：./build.ps1 -WithTests
#   指定构建配置：./build.ps1 -Config Debug

param(
    [string]$Config = 'RelWithDebInfo',
    [string]$BuildDir = 'out/build/app',
    [string]$Generator = 'Visual Studio 17 2022',
    [switch]$Clean,
    [switch]$WithTests
)

$ErrorActionPreference = 'Stop'
$projectDirectory = $PSScriptRoot
$buildPath = Join-Path $projectDirectory $BuildDir

$validConfigs = @('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel')
if ($Config -notin $validConfigs) {
    throw "不支持的构建配置：$Config（可选：$($validConfigs -join ' / ')）"
}
if (!(Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw '未找到 cmake，请安装 CMake（>= 3.16）并加入 PATH。'
}
if (!(Test-Path -LiteralPath (Join-Path $projectDirectory 'crow_all.h'))) {
    throw '缺少 crow_all.h，请确认在项目根目录执行本脚本。'
}
if (!(Test-Path -LiteralPath (Join-Path $projectDirectory 'third_party\boost_1_78_0\boost\version.hpp'))) {
    throw '缺少 Boost 头文件：把 boost_1_78_0 解压到 third_party/boost_1_78_0，正确结构应为 third_party\boost_1_78_0\boost\version.hpp。'
}

if ($Clean -and (Test-Path -LiteralPath $buildPath)) {
    Write-Host "清理构建目录：$BuildDir"
    Remove-Item -LiteralPath $buildPath -Recurse -Force
}

if (!(Test-Path -LiteralPath (Join-Path $buildPath 'CMakeCache.txt'))) {
    Write-Host "配置 CMake -> $BuildDir"
    $configureArgs = @('-S', $projectDirectory, '-B', $buildPath, '-G', $Generator)
    if ($Generator -like 'Visual Studio*') { $configureArgs += @('-A', 'x64') }
    cmake @configureArgs
    if ($LASTEXITCODE -ne 0) {
        throw 'CMake 配置失败：常见原因是未安装 MySQL 8.0 客户端，或未找到 Visual Studio 2022 的 C++ 工具集。'
    }
}

$targets = @('browser_chatroom_architecture')
if ($WithTests) { $targets += @('session_lifecycle_test', 'session_socket_server') }

Write-Host "编译目标：$($targets -join ', ')（$Config）"
cmake --build $buildPath --config $Config --target $targets
if ($LASTEXITCODE -ne 0) {
    throw '编译失败。如果提示 LNK1168，说明后端正在运行，先停止它再重试。'
}

$exe = Join-Path $buildPath "$Config/browser_chatroom_architecture.exe"
if (!(Test-Path -LiteralPath $exe)) {
    throw "未找到构建产物：$exe"
}
Write-Host "构建完成（未启动服务）：$exe"
Write-Host '下一步：./start-backend.ps1 启动服务（首次使用只需跑一次 build.ps1）'