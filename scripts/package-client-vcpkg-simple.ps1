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

# Resolve exe path (VS puts it in Release\ or RelWithDebInfo\, Ninja puts it in client\desktop\)
$exeFullPath = $null
$exeCandidates = @(
    (Join-Path (Get-Location) $exeRelPath),
    (Join-Path (Get-Location) ('client\desktop\Release\' + [IO.Path]::GetFileName($exeRelPath))),
    (Join-Path (Get-Location) ('client\desktop\RelWithDebInfo\' + [IO.Path]::GetFileName($exeRelPath))),
    (Join-Path (Get-Location) ('client\desktop\MinSizeRel\' + [IO.Path]::GetFileName($exeRelPath)))
)
foreach ($p in $exeCandidates) {
    if (Test-Path $p) {
        $exeFullPath = $p
        break
    }
}
if (-not $exeFullPath) {
    Write-Host ('Executable not found. Tried: ' + ($exeCandidates -join '; ')) -ForegroundColor Red
    exit 1
}
Write-Host ('Build succeeded: ' + $exeFullPath) -ForegroundColor Green

# ----- package to one folder: dist\SwiftChat-<triplet>\ -----
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
Write-Host '[3/3] Packaging: all files into one folder' -ForegroundColor Green
Write-Host ('  Folder: ' + $distDir) -ForegroundColor Cyan

# 1) Copy executable
Copy-Item $exeFullPath $distDir
Write-Host '  - SwiftChat.exe copied' -ForegroundColor Gray

# 2) Run windeployqt so Qt DLLs and plugins (e.g. platforms\) go into this same folder
$toolsDir = Join-Path (Join-Path $vcpkgRoot 'installed') $triplet
$windeployqt = $null
if (Test-Path $toolsDir) {
    $windeployqt = Get-ChildItem -Path $toolsDir -Recurse -Filter 'windeployqt.exe' -ErrorAction SilentlyContinue | Select-Object -First 1
}
if ($windeployqt) {
    Write-Host ('  - Running windeployqt (Qt runtime into same folder)...') -ForegroundColor Gray
    & $windeployqt.FullName (Join-Path $distDir 'SwiftChat.exe')
    if ($LASTEXITCODE -eq 0) {
        Write-Host '    windeployqt done' -ForegroundColor Gray
    } else {
        Write-Host '    windeployqt failed; will copy DLLs from vcpkg bin' -ForegroundColor Yellow
    }
} else {
    Write-Host '  - windeployqt not found; copying vcpkg bin DLLs' -ForegroundColor Yellow
}

# 3) Copy ALL DLLs from vcpkg installed (bin + tools subdirs like Qt) into the same folder
$vcpkgBin = Join-Path $vcpkgInstalled 'bin'
if (Test-Path $vcpkgBin) {
    $dlls = Get-ChildItem -Path $vcpkgBin -Filter '*.dll' -ErrorAction SilentlyContinue
    $dlls | ForEach-Object { Copy-Item $_.FullName $distDir -Force -ErrorAction SilentlyContinue }
    Write-Host ('  - Copied ' + $dlls.Count + ' DLL(s) from vcpkg installed\...\bin') -ForegroundColor Gray
}
$vcpkgTools = Join-Path $vcpkgInstalled 'tools'
if (Test-Path $vcpkgTools) {
    $dlls = Get-ChildItem -Path $vcpkgTools -Recurse -Filter '*.dll' -ErrorAction SilentlyContinue
    $dlls | ForEach-Object { Copy-Item $_.FullName $distDir -Force -ErrorAction SilentlyContinue }
    Write-Host ('  - Copied ' + $dlls.Count + ' DLL(s) from vcpkg installed\...\tools') -ForegroundColor Gray
}

# 4) Copy DLLs from build vcpkg_installed bin into the same folder
$buildVcpkgBin = Join-Path (Join-Path $projectRoot $buildDir) ('vcpkg_installed\' + $triplet + '\bin')
if (Test-Path $buildVcpkgBin) {
    $count = 0
    Get-ChildItem -Path $buildVcpkgBin -Filter '*.dll' -ErrorAction SilentlyContinue | ForEach-Object {
        Copy-Item $_.FullName $distDir -Force -ErrorAction SilentlyContinue
        $count++
    }
    Write-Host ('  - Copied ' + $count + ' DLL(s) from build vcpkg_installed\...\bin') -ForegroundColor Gray
}

$fileCount = (Get-ChildItem -Path $distDir -File -ErrorAction SilentlyContinue).Count
$dirCount = (Get-ChildItem -Path $distDir -Directory -ErrorAction SilentlyContinue).Count
Write-Host '' -ForegroundColor Green
Write-Host 'Packaging completed. All files are in ONE folder:' -ForegroundColor Green
Write-Host ('  ' + $distDir) -ForegroundColor White
Write-Host ('  (' + $fileCount + ' files, ' + $dirCount + ' subdirs e.g. platforms)') -ForegroundColor Gray
Write-Host '' -ForegroundColor Yellow
Write-Host '  >>> Run SwiftChat.exe from the folder above (dist\...).' -ForegroundColor Yellow
Write-Host '  >>> Do NOT run from build\...\Release - that folder has no DLLs.' -ForegroundColor Yellow
Write-Host '' -ForegroundColor Green
Write-Host 'You can zip this folder and run SwiftChat.exe on any Windows x64 machine.' -ForegroundColor Green

