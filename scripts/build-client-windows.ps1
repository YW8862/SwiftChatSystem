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

# Locate Qt (common local install paths + env vars)
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
    Write-Host "Qt was not found. Please install Qt or set Qt5_DIR." -ForegroundColor Yellow
    Write-Host "  Example: `$env:Qt5_DIR='E:\Qt\5.15.2\mingw81_64'" -ForegroundColor Cyan
    exit 1
}

# If using MinGW Qt, prepend Qt's MinGW bin to PATH
if ($QtPath -match "mingw") {
    $QtBase = Split-Path (Split-Path $QtPath -Parent) -Parent  # e.g. E:\Qt
    $ToolsMingw = Get-ChildItem -Path "$QtBase\Tools" -Directory -Filter "mingw*" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($ToolsMingw -and (Test-Path "$($ToolsMingw.FullName)\bin\g++.exe")) {
        $env:Path = "$($ToolsMingw.FullName)\bin;" + $env:Path
        Write-Host "Added to PATH: $($ToolsMingw.FullName)\bin (MinGW)"
    }
}

# Protobuf via vcpkg.
# MinGW Qt -> x64-mingw-dynamic, MSVC Qt -> x64-windows
$VcpkgRoot = $env:VCPKG_ROOT
if (-not $VcpkgRoot -and (Test-Path "E:\vcpkg")) { $VcpkgRoot = "E:\vcpkg" }
$Triplet = if ($QtPath -match "mingw") { "x64-mingw-dynamic" } else { "x64-windows" }
$InstalledDir = "$VcpkgRoot\installed\$Triplet"
$CmakePrefixPath = $QtPath
$VcpkgToolchain = "$VcpkgRoot\scripts\buildsystems\vcpkg.cmake"
if ($VcpkgRoot -and (Test-Path $InstalledDir)) {
    $CmakePrefixPath = "$QtPath;$InstalledDir"
    Write-Host "Using vcpkg installed packages (triplet=$Triplet): $InstalledDir"
} elseif ($VcpkgRoot) {
    Write-Host "Please run first: vcpkg install protobuf:${Triplet}" -ForegroundColor Yellow
    exit 1
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
    "-DCMAKE_PREFIX_PATH=$CmakePrefixPath",
    "-DSWIFT_BUILD_CLIENT_ONLY=ON",
    "-DSWIFT_BUILD_CLIENT=ON"
)
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
    Write-Host "Before running, deploy Qt DLLs (example):" -ForegroundColor Cyan
    Write-Host "  $QtPath\bin\windeployqt.exe $BuildDir\$ExePath" -ForegroundColor Cyan
} else {
    Write-Host "Executable not found." -ForegroundColor Red
    exit 1
}
