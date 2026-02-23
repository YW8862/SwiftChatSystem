# SwiftChatSystem Windows 客户端构建脚本
# 前置要求：
#   1. Qt 5.15 for Windows (建议 Qt Online Installer，勾选 MinGW 或 MSVC 组件)
#   2. CMake 3.16+
#   3. vcpkg（用于 Protobuf）或单独安装 Protobuf
# 用法：在项目根目录执行 .\scripts\build-client-windows.ps1

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
Set-Location $ProjectRoot

# 查找 Qt5（依赖安装在 E 盘）
$QtDirs = @(
    "E:\Qt\5.15.2\msvc2019_64",
    "E:\Qt\5.15.2\mingw81_64",
    "E:\Qt\5.15.3\msvc2019_64",
    "E:\Qt\5.15.3\mingw81_64",
    "E:\Qt\6.5.0\msvc2019_64",
    "E:\Qt\6.5.0\mingw_64",
    $env:Qt5_DIR,
    $env:Qt6_DIR
)
$QtPath = $null
foreach ($d in $QtDirs) {
    if ($d -and (Test-Path "$d\lib\cmake\Qt5")) {
        $QtPath = $d
        break
    }
}
if (-not $QtPath) {
    Write-Host "未找到 Qt5，请设置 Qt5_DIR 或安装 Qt 到 E:\Qt\" -ForegroundColor Yellow
    Write-Host "示例: `$env:Qt5_DIR='E:\Qt\5.15.2\mingw81_64'" -ForegroundColor Cyan
    exit 1
}

# 使用 MinGW 版 Qt 时，将 Qt 自带的 MinGW 加入 PATH，以便 CMake 使用 g++
if ($QtPath -match "mingw") {
    $QtBase = Split-Path (Split-Path $QtPath -Parent) -Parent  # e.g. E:\Qt
    $ToolsMingw = Get-ChildItem -Path "$QtBase\Tools" -Directory -Filter "mingw*" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($ToolsMingw -and (Test-Path "$($ToolsMingw.FullName)\bin\g++.exe")) {
        $env:Path = "$($ToolsMingw.FullName)\bin;" + $env:Path
        Write-Host "已加入 PATH: $($ToolsMingw.FullName)\bin (MinGW)"
    }
}

# Protobuf: 通过 vcpkg 获取。Qt 为 MinGW 时用 x64-mingw-dynamic，为 MSVC 时用 x64-windows
$VcpkgRoot = $env:VCPKG_ROOT
if (-not $VcpkgRoot -and (Test-Path "E:\vcpkg")) { $VcpkgRoot = "E:\vcpkg" }
$Triplet = if ($QtPath -match "mingw") { "x64-mingw-dynamic" } else { "x64-windows" }
$InstalledDir = "$VcpkgRoot\installed\$Triplet"
$CmakePrefixPath = $QtPath
$VcpkgToolchain = "$VcpkgRoot\scripts\buildsystems\vcpkg.cmake"
if ($VcpkgRoot -and (Test-Path $InstalledDir)) {
    $CmakePrefixPath = "$QtPath;$InstalledDir"
    Write-Host "使用 vcpkg 已安装包 (triplet=$Triplet): $InstalledDir"
} elseif ($VcpkgRoot) {
    Write-Host "请先执行: vcpkg install protobuf:${Triplet}" -ForegroundColor Yellow
    exit 1
}

# 构建
$BuildDir = "build-windows"
if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
New-Item -ItemType Directory -Path $BuildDir | Out-Null
Set-Location $BuildDir

$CmakeArgs = @(
    "..",
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_PREFIX_PATH=$CmakePrefixPath",
    "-DSWIFT_BUILD_CLIENT_ONLY=ON",
    "-DSWIFT_BUILD_CLIENT=ON"
)

Write-Host "配置 CMake..." -ForegroundColor Green
& cmake @CmakeArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "编译 SwiftChat..." -ForegroundColor Green
& ninja SwiftChat
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$ExePath = "client\desktop\SwiftChat.exe"
if (Test-Path $ExePath) {
    Write-Host "`n构建成功: $BuildDir\$ExePath" -ForegroundColor Green
    Write-Host "运行前需将 Qt DLL 复制到同目录，或使用 windeployqt:" -ForegroundColor Cyan
    Write-Host "  $QtPath\bin\windeployqt.exe $BuildDir\$ExePath" -ForegroundColor Cyan
} else {
    Write-Host "未找到可执行文件" -ForegroundColor Red
    exit 1
}
