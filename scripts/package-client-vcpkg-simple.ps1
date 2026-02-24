# SwiftChatSystem Windows client one-click build & package (vcpkg + Qt)
# Assumptions:
#   - You are on Windows 11 x64
#   - VCPKG_ROOT is set (e.g. E:\vcpkg)
#   - qt5-base and protobuf are installed for your triplet, e.g.:
#       vcpkg install qt5-base:x64-windows protobuf:x64-windows
#
# Usage (from project root):
#   powershell -ExecutionPolicy Bypass -File .\scripts\package-client-vcpkg-simple.ps1

$ErrorActionPreference = 'Stop'

# ----- locate project root -----
$projectRoot = Split-Path -Parent $PSScriptRoot
Set-Location $projectRoot

# ----- vcpkg config -----
$vcpkgRoot = $env:VCPKG_ROOT
if (-not $vcpkgRoot) {
    Write-Host 'VCPKG_ROOT is not set. Please set it first, for example:' -ForegroundColor Red
    Write-Host '  $env:VCPKG_ROOT = ''E:\vcpkg''' -ForegroundColor Yellow
    exit 1
}

$triplet = $env:VCPKG_TRIPLET
if (-not $triplet) {
    if ($env:VCPKG_DEFAULT_TRIPLET) {
        $triplet = $env:VCPKG_DEFAULT_TRIPLET
    } else {
        $triplet = 'x64-windows'
    }
}

$toolchain = Join-Path $vcpkgRoot 'scripts\buildsystems\vcpkg.cmake'
if (-not (Test-Path $toolchain)) {
    Write-Host ('vcpkg toolchain not found: ' + $toolchain) -ForegroundColor Red
    exit 1
}

$vcpkgInstalled = Join-Path (Join-Path $vcpkgRoot 'installed') $triplet
if (-not (Test-Path $vcpkgInstalled)) {
    Write-Host ('vcpkg installed dir not found: ' + $vcpkgInstalled) -ForegroundColor Red
    exit 1
}

Write-Host ('Project root : ' + $projectRoot) -ForegroundColor Cyan
Write-Host ('VCPKG_ROOT  : ' + $vcpkgRoot) -ForegroundColor Cyan
Write-Host ('Triplet     : ' + $triplet) -ForegroundColor Cyan

# ----- build configuration -----
$buildDir   = 'build-vcpkg-' + $triplet
$exeRelPath = 'client\desktop\SwiftChat.exe'

if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}
Set-Location $buildDir

function Get-Generator {
    # Prefer Ninja if available; otherwise fall back to a generic generator.
    if (Get-Command ninja -ErrorAction SilentlyContinue) {
        return 'Ninja'
    }
    # Fallback: let CMake pick a default generator (usually VS) by not specifying -G explicitly.
    return ''
}

$gen = Get-Generator
if ($gen -ne '') {
    Write-Host ('Using CMake generator: ' + $gen) -ForegroundColor Green
    $cmakeArgs = @(
        '..',
        '-G', $gen,
        '-DCMAKE_BUILD_TYPE=Release',
        ('-DCMAKE_TOOLCHAIN_FILE=' + $toolchain),
        ('-DCMAKE_PREFIX_PATH=' + $vcpkgInstalled),
        ('-DVCPKG_TARGET_TRIPLET=' + $triplet),
        '-DSWIFT_BUILD_CLIENT_ONLY=ON',
        '-DSWIFT_BUILD_CLIENT=ON'
    )
} else {
    Write-Host 'Using CMake default generator (no -G specified).' -ForegroundColor Green
    $cmakeArgs = @(
        '..',
        '-DCMAKE_BUILD_TYPE=Release',
        ('-DCMAKE_TOOLCHAIN_FILE=' + $toolchain),
        ('-DCMAKE_PREFIX_PATH=' + $vcpkgInstalled),
        ('-DVCPKG_TARGET_TRIPLET=' + $triplet),
        '-DSWIFT_BUILD_CLIENT_ONLY=ON',
        '-DSWIFT_BUILD_CLIENT=ON'
    )
}

Write-Host '' -ForegroundColor Green
Write-Host '[1/3] Configure CMake (Release)...' -ForegroundColor Green
cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host '' -ForegroundColor Green
Write-Host '[2/3] Build SwiftChat (Release)...' -ForegroundColor Green
if ($gen -eq 'Ninja') {
    ninja SwiftChat
} else {
    cmake --build . --config Release --target SwiftChat
}
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if (-not (Test-Path $exeRelPath)) {
    Write-Host ('Executable not found: ' + $exeRelPath) -ForegroundColor Red
    exit 1
}

$exeFullPath = Join-Path (Get-Location) $exeRelPath
Write-Host ('Build succeeded: ' + $exeFullPath) -ForegroundColor Green

# ----- package to dist -----
Set-Location $projectRoot
$distRoot = 'dist'
if (-not (Test-Path $distRoot)) {
    New-Item -ItemType Directory -Path $distRoot | Out-Null
}

$distDirName = 'SwiftChat-' + $triplet
$distDir = Join-Path $distRoot $distDirName
if (Test-Path $distDir) {
    Remove-Item -Recurse -Force $distDir
}
New-Item -ItemType Directory -Path $distDir | Out-Null

Write-Host '' -ForegroundColor Green
Write-Host ('[3/3] Copy executable to: ' + $distDir) -ForegroundColor Green
Copy-Item $exeFullPath $distDir

# ----- try windeployqt from vcpkg -----
$windeployqt = $null
$toolsDir = Join-Path (Join-Path $vcpkgRoot 'installed') $triplet
if (Test-Path $toolsDir) {
    $windeployqt = Get-ChildItem -Path $toolsDir -Recurse -Filter 'windeployqt.exe' -ErrorAction SilentlyContinue | Select-Object -First 1
}

if ($windeployqt) {
    Write-Host ('Found windeployqt: ' + $windeployqt.FullName) -ForegroundColor Cyan
    Write-Host 'Running windeployqt to deploy Qt runtime...' -ForegroundColor Cyan
    & $windeployqt.FullName (Join-Path $distDir 'SwiftChat.exe')
    if ($LASTEXITCODE -ne 0) {
        Write-Host 'windeployqt failed; will still copy DLLs from vcpkg bin.' -ForegroundColor Yellow
    }
} else {
    Write-Host 'windeployqt.exe not found; will copy DLLs from vcpkg bin only.' -ForegroundColor Yellow
}

# ----- copy DLLs from vcpkg bin (fallback) -----
$binDir = Join-Path $toolsDir 'bin'
if (Test-Path $binDir) {
    Write-Host ('Copying DLLs from vcpkg bin: ' + $binDir) -ForegroundColor Cyan
    Get-ChildItem -Path $binDir -Filter '*.dll' -ErrorAction SilentlyContinue | ForEach-Object {
        Copy-Item $_.FullName $distDir -ErrorAction SilentlyContinue
    }
} else {
    Write-Host ('vcpkg bin directory not found: ' + $binDir + ' . Please verify triplet and VCPKG_ROOT.') -ForegroundColor Yellow
}

Write-Host '' -ForegroundColor Green
Write-Host 'Packaging completed.' -ForegroundColor Green
Write-Host ('Output directory: ' + $distDir) -ForegroundColor Green
Write-Host 'You can zip the whole folder and run SwiftChat.exe directly on target machines.' -ForegroundColor Green

