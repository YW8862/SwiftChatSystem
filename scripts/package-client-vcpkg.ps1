# SwiftChatSystem Windows 客户端一键打包脚本（基于 vcpkg Qt）
# 适用场景：已在 Windows 11 x64 上通过 vcpkg 安装 qt5-base 等依赖
#   - 已安装示例：vcpkg install qt5-base:x64-windows protobuf:x64-windows
#   - 或对应的其它 triplet（修改 $Triplet 即可）
#
# 功能：
#   1. 使用 vcpkg 工具链配置并构建 SwiftChat 客户端 (Release)
#   2. 将可执行文件和依赖 DLL 打包到 dist\SwiftChat-<triplet>\ 目录
#   3. 若找到 windeployqt，则自动调用以补齐 Qt 运行时
#
# 用法：在项目根目录执行：
#   powershell -ExecutionPolicy Bypass -File .\scripts\package-client-vcpkg.ps1

$ErrorActionPreference = "Stop"

# -------- 基本路径与参数 --------
$ProjectRoot = Split-Path -Parent $PSScriptRoot
Set-Location $ProjectRoot

# vcpkg 根目录与 triplet：
#   - 默认从 VCPKG_ROOT 读取
#   - Triplet 默认 x64-windows（常见 MSVC 环境），如需 MinGW 请自行调整
$VcpkgRoot = $env:VCPKG_ROOT
if (-not $VcpkgRoot) {
    Write-Host "未检测到 VCPKG_ROOT 环境变量，请先设置，例如：" -ForegroundColor Red
    Write-Host "  `$env:VCPKG_ROOT = 'E:\vcpkg'" -ForegroundColor Yellow
    Write-Host "或者在系统/用户环境变量中配置 VCPKG_ROOT。" -ForegroundColor Yellow
    exit 1
}

$Triplet = if ($env:VCPKG_DEFAULT_TRIPLET) { $env:VCPKG_DEFAULT_TRIPLET } else { "x64-windows" }
if ($env:VCPKG_TRIPLET) { $Triplet = $env:VCPKG_TRIPLET }

$ToolchainFile = Join-Path $VcpkgRoot "scripts\buildsystems\vcpkg.cmake"
if (-not (Test-Path $ToolchainFile)) {
    Write-Host "未找到 vcpkg 工具链文件：$ToolchainFile" -ForegroundColor Red
    exit 1
}

Write-Host "项目根目录: $ProjectRoot" -ForegroundColor Cyan
Write-Host "VCPKG_ROOT : $VcpkgRoot" -ForegroundColor Cyan
Write-Host "Triplet    : $Triplet" -ForegroundColor Cyan

# -------- 构建配置 --------
$BuildDir = "build-vcpkg-$Triplet"
$ExeRelPath = "client\desktop\SwiftChat.exe"

if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}
Set-Location $BuildDir

# 选择生成器：若已安装 Ninja，优先使用 Ninja；否则使用 VS 生成器
function Get-Generator {
    if (Get-Command ninja -ErrorAction SilentlyContinue) {
        return @("Ninja")
    }
    # 尝试 VS 2022，再尝试 VS 2019
    $generators = @(
        "Visual Studio 17 2022",
        "Visual Studio 16 2019"
    )
    foreach ($g in $generators) {
        $out = cmake -G "$g" 2>&1
        if ($LASTEXITCODE -eq 0) {
            return @($g)
        }
    }
    Write-Host "未找到合适的 CMake 生成器，请安装 Ninja 或 Visual Studio 2019/2022。" -ForegroundColor Red
    exit 1
}

$gen = Get-Generator

Write-Host "使用 CMake 生成器: $($gen[0])" -ForegroundColor Green

$CmakeArgs = @(
    "..",
    "-G", $gen[0],
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_TOOLCHAIN_FILE=$ToolchainFile",
    "-DVCPKG_TARGET_TRIPLET=$Triplet",
    "-DSWIFT_BUILD_CLIENT_ONLY=ON",
    "-DSWIFT_BUILD_CLIENT=ON"
)

Write-Host "`n[1/3] 配置 CMake..." -ForegroundColor Green
& cmake @CmakeArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "`n[2/3] 编译 SwiftChat (Release)..." -ForegroundColor Green
if ($gen[0] -eq "Ninja") {
    & ninja SwiftChat
} else {
    # VS 生成器：使用 cmake --build
    & cmake --build . --config Release --target SwiftChat
}
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if (-not (Test-Path $ExeRelPath)) {
    Write-Host "未找到可执行文件: $ExeRelPath" -ForegroundColor Red
    exit 1
}

$ExeFullPath = Join-Path (Get-Location) $ExeRelPath
Write-Host "`n构建成功: $ExeFullPath" -ForegroundColor Green

# -------- 打包到 dist --------
Set-Location $ProjectRoot
$DistRoot = "dist"
if (-not (Test-Path $DistRoot)) {
    New-Item -ItemType Directory -Path $DistRoot | Out-Null
}

$DistDirName = "SwiftChat-$Triplet"
$DistDir = Join-Path $DistRoot $DistDirName
if (Test-Path $DistDir) {
    Remove-Item -Recurse -Force $DistDir
}
New-Item -ItemType Directory -Path $DistDir | Out-Null

Write-Host "`n[3/3] 打包到: $DistDir" -ForegroundColor Green
Copy-Item $ExeFullPath $DistDir

# ---- 查找 windeployqt（若使用 Qt MSVC 版，vcpkg 通常会提供） ----
$Windeployqt = $null
$ToolsDir = Join-Path (Join-Path $VcpkgRoot "installed") $Triplet
if (Test-Path $ToolsDir) {
    $Windeployqt = Get-ChildItem -Path $ToolsDir -Recurse -Filter "windeployqt.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
}

if ($Windeployqt) {
    Write-Host "Found windeployqt: $($Windeployqt.FullName)" -ForegroundColor Cyan
    Write-Host "Running windeployqt to deploy Qt runtime..." -ForegroundColor Cyan
    & $Windeployqt.FullName (Join-Path $DistDir "SwiftChat.exe")
    if ($LASTEXITCODE -ne 0) {
        Write-Host "windeployqt failed; will still copy DLLs from vcpkg bin." -ForegroundColor Yellow
    }
} else {
    Write-Host "windeployqt.exe not found; will copy DLLs from vcpkg bin." -ForegroundColor Yellow
}

# ---- 从 vcpkg bin 目录复制 DLL（无论是否有 windeployqt，都执行一次，兜底） ----
$BinDir = Join-Path $ToolsDir "bin"
if (Test-Path $BinDir) {
    Write-Host "Copying DLLs from vcpkg bin: $BinDir" -ForegroundColor Cyan
    Copy-Item (Join-Path $BinDir "*.dll") $DistDir -ErrorAction SilentlyContinue
} else {
    Write-Host "vcpkg bin directory not found: $BinDir . Please verify triplet and VCPKG_ROOT." -ForegroundColor Yellow
}

Write-Host "" -ForegroundColor Green
Write-Host 'Packaging completed.' -ForegroundColor Green
Write-Host ('Output directory: ' + $DistDir) -ForegroundColor Green
Write-Host 'You can zip the whole folder and run SwiftChat.exe directly on target machines.' -ForegroundColor Green

