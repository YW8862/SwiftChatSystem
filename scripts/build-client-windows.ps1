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

# Select CMake generator automatically
# - mingw triplet: prefer Ninja, fallback to MinGW Makefiles
# - msvc triplet: use Visual Studio generator (no Ninja required)
$Generator = $env:CMAKE_GENERATOR
if (-not $Generator) {
    if ($Triplet -like "*mingw*") {
        if (Get-Command ninja -ErrorAction SilentlyContinue) {
            $Generator = "Ninja"
        } else {
            $Generator = "MinGW Makefiles"
        }
    } else {
        # MSVC path: detect VS 2022/2019 via vswhere if available
        $Generator = "Visual Studio 17 2022"
        $VsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
        if (Test-Path $VsWhere) {
            $VsInstall = & $VsWhere -latest -products * -requires Microsoft.Component.MSBuild -property installationVersion 2>$null
            if ($LASTEXITCODE -eq 0 -and $VsInstall) {
                if ($VsInstall.StartsWith("16.")) {
                    $Generator = "Visual Studio 16 2019"
                } else {
                    $Generator = "Visual Studio 17 2022"
                }
            }
        }
    }
}
Write-Host "Using CMake generator: $Generator" -ForegroundColor Green

$CmakeArgs = @(
    "..",
    "-G", $Generator,
    "-DSWIFT_BUILD_CLIENT_ONLY=ON",
    "-DSWIFT_BUILD_CLIENT=ON"
)
# Single-config generators need CMAKE_BUILD_TYPE
if ($Generator -notmatch "Visual Studio") {
    $CmakeArgs += "-DCMAKE_BUILD_TYPE=Release"
}
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
if ($Generator -eq "Ninja") {
    & ninja SwiftChat
} elseif ($Generator -match "Visual Studio") {
    & cmake --build . --config Release --target SwiftChat
} else {
    & cmake --build . --target SwiftChat -- -j4
}
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$ExeCandidates = @(
    "client\desktop\SwiftChat.exe",          # Ninja / single-config
    "client\desktop\Release\SwiftChat.exe",  # Visual Studio multi-config
    "client\desktop\Debug\SwiftChat.exe"
)
$ExePath = $null
foreach ($p in $ExeCandidates) {
    if (Test-Path $p) {
        $ExePath = $p
        break
    }
}
if (-not $ExePath) {
    $Found = Get-ChildItem -Path "client\desktop" -Recurse -Filter "SwiftChat.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($Found) {
        $ExePath = $Found.FullName
    }
}

if ($ExePath) {
    # Normalize exe full path
    if (Test-Path $ExePath) {
        $ExePath = (Resolve-Path $ExePath).Path
    }
    Write-Host "`nBuild succeeded: $ExePath" -ForegroundColor Green

    # -------------------------------------------------------------------------
    # Deploy runtime deps to dist folder
    # -------------------------------------------------------------------------
    $DistRoot = Join-Path $ProjectRoot "dist"
    if (-not (Test-Path $DistRoot)) { New-Item -ItemType Directory -Path $DistRoot | Out-Null }
    $DistBaseName = "SwiftChat-$Triplet"
    $DistDir = Join-Path $DistRoot $DistBaseName

    # If previous packaged app is still running, files like qwindows.dll may be locked.
    Get-Process -Name "SwiftChat" -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue

    if (Test-Path $DistDir) {
        try {
            Remove-Item -Recurse -Force $DistDir -ErrorAction Stop
        } catch {
            $Suffix = Get-Date -Format "yyyyMMdd-HHmmss"
            $DistDir = Join-Path $DistRoot "$DistBaseName-$Suffix"
            Write-Host "Existing dist folder is locked, using new folder: $DistDir" -ForegroundColor Yellow
        }
    }
    New-Item -ItemType Directory -Path $DistDir | Out-Null

    Write-Host "Packaging runtime to: $DistDir" -ForegroundColor Green
    Copy-Item $ExePath $DistDir -Force

    # Find windeployqt from standalone Qt or vcpkg tools
    $DeployQtPath = $null
    if ($QtPath -and (Test-Path "$QtPath\bin\windeployqt.exe")) {
        $DeployQtPath = "$QtPath\bin\windeployqt.exe"
    } elseif ($InstalledDir) {
        $DeployQt = Get-ChildItem -Path "$InstalledDir\tools" -Recurse -Filter "windeployqt.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($DeployQt) { $DeployQtPath = $DeployQt.FullName }
    }

    if ($DeployQtPath) {
        Write-Host "Running windeployqt: $DeployQtPath" -ForegroundColor Cyan
        & $DeployQtPath (Join-Path $DistDir "SwiftChat.exe")
        if ($LASTEXITCODE -ne 0) {
            Write-Host "windeployqt failed; continue with vcpkg DLL copy fallback." -ForegroundColor Yellow
        }
    } else {
        Write-Host "windeployqt not found; continue with vcpkg DLL copy fallback." -ForegroundColor Yellow
    }

    # Copy ALL vcpkg runtime DLLs (Qt5, Qt5WebSockets, protobuf, abseil, etc.)
    $VcpkgBin = if ($InstalledDir -and (Test-Path "$InstalledDir\bin")) { "$InstalledDir\bin" } else { $null }
    if ($VcpkgBin) {
        Write-Host "Copying all DLLs from: $VcpkgBin" -ForegroundColor Cyan
        Get-ChildItem -Path $VcpkgBin -Filter "*.dll" -ErrorAction SilentlyContinue | ForEach-Object {
            Copy-Item $_.FullName $DistDir -Force -ErrorAction SilentlyContinue
        }
    }

    # Copy plugins (platforms/qwindows.dll, imageformats, etc.)
    foreach ($PlugDir in @("$InstalledDir\plugins", "$InstalledDir\share\qt5\plugins", "$InstalledDir\share\qt6\plugins")) {
        if ($PlugDir -and (Test-Path $PlugDir)) {
            Write-Host "Copying plugins from: $PlugDir" -ForegroundColor Cyan
            Copy-Item -Recurse -Force $PlugDir $DistDir -ErrorAction SilentlyContinue
            break
        }
    }

    # Also copy deps into the build Release folder so running from there works
    $ExeDir = Split-Path $ExePath -Parent
    if ($ExeDir -and $ExeDir -ne $DistDir) {
        Write-Host "Copying DLLs to build output: $ExeDir" -ForegroundColor Cyan
        if ($VcpkgBin) {
            Get-ChildItem -Path $VcpkgBin -Filter "*.dll" -ErrorAction SilentlyContinue | ForEach-Object {
                Copy-Item $_.FullName $ExeDir -Force -ErrorAction SilentlyContinue
            }
        }
        foreach ($PlugDir in @("$InstalledDir\plugins", "$InstalledDir\share\qt5\plugins", "$InstalledDir\share\qt6\plugins")) {
            if ($PlugDir -and (Test-Path $PlugDir)) {
                Copy-Item -Recurse -Force $PlugDir $ExeDir -ErrorAction SilentlyContinue
                break
            }
        }
    }

    Write-Host "Package ready: $DistDir" -ForegroundColor Green
    Write-Host "Run: $DistDir\SwiftChat.exe" -ForegroundColor Green
} else {
    Write-Host "Executable not found." -ForegroundColor Red
    exit 1
}
