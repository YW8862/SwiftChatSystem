# SwiftChatSystem Windows client build script
# Requirements:
#   1. Qt 5.15+ for Windows (MinGW or MSVC)
#   2. CMake 3.16+
#   3. vcpkg (for protobuf) or a local protobuf install
# Usage: run from repository root:
#   .\scripts\build-client-windows.ps1

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
Set-Location $ProjectRoot

# Detect vcpkg first (default E:\vcpkg)
$VcpkgRoot = $env:VCPKG_ROOT
if (-not $VcpkgRoot -and (Test-Path "E:\vcpkg")) { $VcpkgRoot = "E:\vcpkg" }
if (-not $VcpkgRoot) {
    Write-Host "VCPKG_ROOT is not set and E:\vcpkg was not found." -ForegroundColor Yellow
    Write-Host "Set it, for example: `$env:VCPKG_ROOT='E:\vcpkg'" -ForegroundColor Cyan
}

# Locate standalone Qt (optional if vcpkg Qt is installed)
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

# Resolve triplet:
# 1) explicit env override, 2) infer from QtPath, 3) fallback x64-windows
$Triplet = $env:VCPKG_TARGET_TRIPLET
if (-not $Triplet) { $Triplet = $env:VCPKG_DEFAULT_TRIPLET }
if (-not $Triplet) {
    if ($QtPath -and ($QtPath -match "mingw")) { $Triplet = "x64-mingw-dynamic" }
    else { $Triplet = "x64-windows" }
}

$InstalledDir = $null
$VcpkgToolchain = $null
if ($VcpkgRoot) {
    $InstalledDir = "$VcpkgRoot\installed\$Triplet"
    $VcpkgToolchain = "$VcpkgRoot\scripts\buildsystems\vcpkg.cmake"
}

# Compose CMAKE_PREFIX_PATH from available roots
$PrefixItems = @()
if ($QtPath) { $PrefixItems += $QtPath }
if ($InstalledDir -and (Test-Path $InstalledDir)) { $PrefixItems += $InstalledDir }
$CmakePrefixPath = ($PrefixItems -join ";")

if (-not $QtPath -and -not ($InstalledDir -and (Test-Path "$InstalledDir\share\qt5"))) {
    Write-Host "Qt not found in standalone paths and not found in vcpkg share\qt5." -ForegroundColor Yellow
    if ($VcpkgRoot) {
        Write-Host "Try: $VcpkgRoot\vcpkg install qt5-base:$Triplet qt5-websockets:$Triplet" -ForegroundColor Cyan
    } else {
        Write-Host "Install standalone Qt or configure VCPKG_ROOT." -ForegroundColor Cyan
    }
    exit 1
}

if ($QtPath) {
    Write-Host "Using standalone Qt: $QtPath"
}
if ($InstalledDir -and (Test-Path $InstalledDir)) {
    Write-Host "Using vcpkg packages (triplet=$Triplet): $InstalledDir"
}

# If using MinGW standalone Qt, prepend Qt MinGW bin to PATH
if ($QtPath -and ($QtPath -match "mingw")) {
    $QtBase = Split-Path (Split-Path $QtPath -Parent) -Parent
    $ToolsMingw = Get-ChildItem -Path "$QtBase\Tools" -Directory -Filter "mingw*" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($ToolsMingw -and (Test-Path "$($ToolsMingw.FullName)\bin\g++.exe")) {
        $env:Path = "$($ToolsMingw.FullName)\bin;" + $env:Path
        Write-Host "Added to PATH: $($ToolsMingw.FullName)\bin (MinGW)"
    }
}

# Build
$BuildDir = "build-windows"
if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
New-Item -ItemType Directory -Path $BuildDir | Out-Null
Set-Location $BuildDir

$CmakeArgs = @(
    "..",
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DSWIFT_BUILD_CLIENT_ONLY=ON",
    "-DSWIFT_BUILD_CLIENT=ON"
)
if ($CmakePrefixPath) {
    $CmakeArgs += "-DCMAKE_PREFIX_PATH=$CmakePrefixPath"
}
if ($VcpkgRoot -and (Test-Path $VcpkgToolchain)) {
    $CmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$VcpkgToolchain"
    $CmakeArgs += "-DVCPKG_TARGET_TRIPLET=$Triplet"
}

Write-Host "Configuring CMake..." -ForegroundColor Green
& cmake @CmakeArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Building SwiftChat..." -ForegroundColor Green
& ninja SwiftChat
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$ExePath = "client\desktop\SwiftChat.exe"
if (Test-Path $ExePath) {
    Write-Host "`nBuild succeeded: $BuildDir\$ExePath" -ForegroundColor Green
    Write-Host "Before running, deploy Qt DLLs (windeployqt)." -ForegroundColor Cyan
    if ($QtPath -and (Test-Path "$QtPath\bin\windeployqt.exe")) {
        Write-Host "  $QtPath\bin\windeployqt.exe $BuildDir\$ExePath" -ForegroundColor Cyan
    } elseif ($InstalledDir) {
        $DeployQt = Get-ChildItem -Path "$InstalledDir\tools" -Recurse -Filter "windeployqt.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($DeployQt) {
            Write-Host "  $($DeployQt.FullName) $BuildDir\$ExePath" -ForegroundColor Cyan
        } else {
            Write-Host "  windeployqt.exe not found automatically. Use your Qt installation path." -ForegroundColor Yellow
        }
    }
} else {
    Write-Host "Executable not found." -ForegroundColor Red
    exit 1
}
