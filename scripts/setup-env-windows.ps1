#Requires -RunAsAdministrator
# SwiftChatSystem Windows 11 x64 构建环境准备脚本
# 以管理员身份运行 PowerShell，在项目根目录或任意目录执行均可
# 用法: .\scripts\setup-env-windows.ps1  或  powershell -ExecutionPolicy Bypass -File scripts\setup-env-windows.ps1

$ErrorActionPreference = "Stop"

$VcpkgRoot = "E:\vcpkg"
$QtRoot = "E:\Qt"

function Write-Step { param($Msg) Write-Host "`n===> $Msg" -ForegroundColor Cyan }
function Write-Ok   { param($Msg) Write-Host "  [OK] $Msg" -ForegroundColor Green }
function Write-Warn { param($Msg) Write-Host "  [!!] $Msg" -ForegroundColor Yellow }
function Write-Err  { param($Msg) Write-Host "  [X] $Msg" -ForegroundColor Red }

# ---------- 1. 检查/安装 winget ----------
Write-Step "检查 winget（用于安装 Git/CMake/Ninja/Python）"
if (-not (Get-Command winget -ErrorAction SilentlyContinue)) {
    Write-Err "未找到 winget。请安装「应用安装程序」或升级 Windows 11 到最新。"
    Write-Host "  可选：从 https://github.com/microsoft/winget-cli/releases 下载安装。" -ForegroundColor Gray
    exit 1
}
Write-Ok "winget 可用"

# ---------- 2. 安装基础构建工具（若未安装） ----------
$WingetPackages = @(
    @{ Id = "Git.Git";           Name = "Git" },
    @{ Id = "Kitware.CMake";     Name = "CMake" },
    @{ Id = "Ninja-build.Ninja"; Name = "Ninja" },
    @{ Id = "Python.Python.3.12"; Name = "Python 3.12" }
)
foreach ($p in $WingetPackages) {
    Write-Step "检查/安装 $($p.Name)"
    $r = winget list --id $p.Id 2>$null
    if ($LASTEXITCODE -ne 0 -or $r -notmatch $p.Id) {
        winget install --id $p.Id -e --accept-source-agreements --accept-package-agreements
        if ($LASTEXITCODE -ne 0) { Write-Err "安装 $($p.Name) 失败"; exit 1 }
        Write-Ok "已安装 $($p.Name)"
    } else {
        Write-Ok "已存在 $($p.Name)"
    }
}

# 刷新 PATH（以便当前会话能找到 cmake、ninja、git、pip）
$env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")

# ---------- 3. vcpkg ----------
Write-Step "配置 vcpkg"
if (-not (Test-Path $VcpkgRoot)) {
    Write-Host "  克隆 vcpkg 到 $VcpkgRoot ..."
    git clone https://github.com/microsoft/vcpkg.git $VcpkgRoot
    if ($LASTEXITCODE -ne 0) { Write-Err "克隆 vcpkg 失败"; exit 1 }
    Write-Ok "已克隆 vcpkg"
} else {
    Write-Ok "vcpkg 目录已存在: $VcpkgRoot"
}

& "$VcpkgRoot\bootstrap-vcpkg.bat"
if ($LASTEXITCODE -ne 0) { Write-Err "vcpkg bootstrap 失败"; exit 1 }
Write-Ok "vcpkg 已就绪"

# 安装 Protobuf（MinGW 版本，与下方 Qt MinGW 一致）
Write-Step "安装 Protobuf (x64-mingw-dynamic)"
& "$VcpkgRoot\vcpkg.exe" install protobuf:x64-mingw-dynamic
if ($LASTEXITCODE -ne 0) { Write-Err "安装 Protobuf 失败"; exit 1 }
Write-Ok "Protobuf 已安装"

# 设置环境变量（当前用户，持久）
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", $VcpkgRoot, "User")
$env:VCPKG_ROOT = $VcpkgRoot
Write-Ok "已设置 VCPKG_ROOT=$VcpkgRoot"

# ---------- 4. Qt 5.15 (MinGW) ----------
Write-Step "配置 Qt 5.15 (MinGW 64-bit)"
$Qt515Path = "$QtRoot\5.15.2\mingw81_64"
$Qt515Cmake = "$Qt515Path\lib\cmake\Qt5"
if (Test-Path $Qt515Cmake) {
    Write-Ok "Qt 5.15.2 MinGW 已存在: $Qt515Path"
} else {
    # 使用 aqtinstall 安装
    $pip = Get-Command pip -ErrorAction SilentlyContinue
    if (-not $pip) { $pip = Get-Command pip3 -ErrorAction SilentlyContinue }
    if (-not $pip) {
        Write-Warn "未找到 pip，无法自动安装 Qt。请手动安装 Qt 5.15.2 MinGW："
        Write-Host "  1. 打开 https://www.qt.io/download-qt-installer" -ForegroundColor Gray
        Write-Host "  2. 下载 Qt Online Installer，安装时勾选 Qt 5.15.2 -> MinGW 8.1.0 64-bit" -ForegroundColor Gray
        Write-Host "  3. 安装路径建议: $QtRoot" -ForegroundColor Gray
        Write-Host "  安装完成后重新运行本脚本或直接执行构建。" -ForegroundColor Gray
    } else {
        Write-Host "  使用 aqtinstall 安装 Qt 5.15.2 MinGW（约 500MB，需数分钟）..."
        & pip install aqtinstall --quiet
        New-Item -ItemType Directory -Force -Path $QtRoot | Out-Null
        & aqt install-qt windows desktop 5.15.2 win64_mingw -O $QtRoot
        if ($LASTEXITCODE -ne 0) { Write-Err "aqt 安装 Qt 失败"; exit 1 }
        # aqt 可能生成 5.15.2\mingw81_64 或类似路径
        $aqtDir = Get-ChildItem -Path "$QtRoot\5.15.2" -Directory -ErrorAction SilentlyContinue | Where-Object { $_.Name -match "mingw" } | Select-Object -First 1
        if ($aqtDir -and (Test-Path "$($aqtDir.FullName)\lib\cmake\Qt5")) {
            Write-Ok "Qt 已安装到 $($aqtDir.FullName)"
        } elseif (Test-Path $Qt515Cmake) {
            Write-Ok "Qt 已安装到 $Qt515Path"
        } else {
            Write-Warn "请确认 Qt 5.15.2 MinGW 已安装到 $QtRoot 下，并存在 lib\cmake\Qt5。"
        }
    }
}

# ---------- 5. 环境变量摘要 ----------
Write-Step "环境变量（建议加入系统或用户 PATH）"
Write-Host "  VCPKG_ROOT = $VcpkgRoot" -ForegroundColor White
if (Test-Path $Qt515Cmake) {
    [Environment]::SetEnvironmentVariable("Qt5_DIR", $Qt515Path, "User")
    $env:Qt5_DIR = $Qt515Path
    Write-Host "  Qt5_DIR   = $Qt515Path" -ForegroundColor White
}
Write-Host ""
Write-Host "下一步：在项目根目录执行构建：" -ForegroundColor Green
Write-Host "  .\scripts\build-client-windows.ps1" -ForegroundColor White
Write-Host ""
Write-Host "若使用新的 PowerShell 窗口，请确保已设置：" -ForegroundColor Yellow
Write-Host "  `$env:VCPKG_ROOT = 'E:\vcpkg'" -ForegroundColor Gray
Write-Host "  `$env:Qt5_DIR    = 'E:\Qt\5.15.2\mingw81_64'   # 若已安装 Qt" -ForegroundColor Gray
Write-Host ""
