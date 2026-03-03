# SwiftChat Windows x64 installer pack script
# 功能：
#   1) 调用 build-client-windows.ps1 构建并生成 dist\SwiftChat-<triplet>
#   2) 使用 Inno Setup 将 dist 目录打包为安装程序（包含全部依赖）
#
# 用法（在项目根目录）：
#   powershell -ExecutionPolicy Bypass -File .\scripts\package-client-installer.ps1
#   powershell -ExecutionPolicy Bypass -File .\scripts\package-client-installer.ps1 -AppVersion 1.2.3 -Publisher "YourCompany"

param(
    [string]$AppName = "SwiftChat",
    [string]$AppVersion = "1.0.0",
    [string]$Publisher = "SwiftChat",
    [string]$IsccPath = "",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$IsWindowsHost = $false
if ($env:OS -eq "Windows_NT") {
    $IsWindowsHost = $true
} else {
    try {
        $IsWindowsHost = [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)
    } catch {
        $IsWindowsHost = $false
    }
}

if (-not $IsWindowsHost) {
    Write-Host "This script must be run on Windows." -ForegroundColor Red
    exit 1
}

$ProjectRoot = Split-Path -Parent $PSScriptRoot
Set-Location $ProjectRoot

function Resolve-Iscc([string]$ManualPath) {
    if ($ManualPath) {
        if (Test-Path $ManualPath) { return $ManualPath }
        Write-Host "Specified ISCC path not found: $ManualPath" -ForegroundColor Yellow
    }

    $candidates = @(
        "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe",
        "$env:LOCALAPPDATA\Programs\Inno Setup 5\ISCC.exe",
        "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
        "${env:ProgramFiles}\Inno Setup 6\ISCC.exe",
        "${env:ProgramFiles(x86)}\Inno Setup 5\ISCC.exe",
        "${env:ProgramFiles}\Inno Setup 5\ISCC.exe"
    )

    foreach ($p in $candidates) {
        if ($p -and (Test-Path $p)) { return $p }
    }

    $cmd = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    # Registry fallback
    foreach ($reg in @(
        "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup 6_is1",
        "HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup 6_is1",
        "HKLM:\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup 6_is1"
    )) {
        try {
            $installLocation = (Get-ItemProperty -Path $reg -ErrorAction Stop).InstallLocation
            if ($installLocation) {
                $exe = Join-Path $installLocation "ISCC.exe"
                if (Test-Path $exe) { return $exe }
            }
        } catch {}
    }

    return $null
}

if (-not $SkipBuild) {
    Write-Host "Step 1/3: building and collecting runtime dependencies..." -ForegroundColor Green
    & (Join-Path $PSScriptRoot "build-client-windows.ps1")
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$DistRoot = Join-Path $ProjectRoot "dist"
if (-not (Test-Path $DistRoot)) {
    Write-Host "dist folder not found: $DistRoot" -ForegroundColor Red
    Write-Host "Please run build-client-windows.ps1 first." -ForegroundColor Yellow
    exit 1
}

$DistCandidates = Get-ChildItem -Path $DistRoot -Directory -Filter "SwiftChat-*" -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending
$DistDir = $null
foreach ($d in $DistCandidates) {
    if (Test-Path (Join-Path $d.FullName "SwiftChat.exe")) {
        $DistDir = $d.FullName
        break
    }
}

if (-not $DistDir) {
    Write-Host "No packaged runtime folder found under dist\SwiftChat-* containing SwiftChat.exe" -ForegroundColor Red
    Write-Host "Please run build-client-windows.ps1 first." -ForegroundColor Yellow
    exit 1
}

$Iscc = Resolve-Iscc $IsccPath
if (-not $Iscc) {
    Write-Host "Inno Setup not found (ISCC.exe)." -ForegroundColor Red
    Write-Host "Install it first, e.g.:" -ForegroundColor Yellow
    Write-Host "  winget install JRSoftware.InnoSetup" -ForegroundColor Cyan
    Write-Host "Or specify path explicitly:" -ForegroundColor Yellow
    Write-Host "  .\scripts\package-client-installer.ps1 -IsccPath ""C:\Users\YANGWEI\AppData\Local\Programs\Inno Setup 6\ISCC.exe""" -ForegroundColor Cyan
    exit 1
}

$InstallerOutDir = Join-Path $ProjectRoot "installer"
if (-not (Test-Path $InstallerOutDir)) {
    New-Item -ItemType Directory -Path $InstallerOutDir | Out-Null
}

$DistName = Split-Path $DistDir -Leaf
$OutputBase = "$AppName-Setup-x64-v$AppVersion"
$IssPath = Join-Path $ProjectRoot "build-windows\package-installer.iss"

if (-not (Test-Path (Split-Path $IssPath -Parent))) {
    New-Item -ItemType Directory -Path (Split-Path $IssPath -Parent) | Out-Null
}

$AppId = "{9B2A8A8D-1E2F-45F0-8B23-8F617F75A4C1}"

$issContent = @"
#define MyAppName "$AppName"
#define MyAppVersion "$AppVersion"
#define MyAppPublisher "$Publisher"
#define MyAppExeName "SwiftChat.exe"
#define MyAppId "$AppId"
#define MySourceDir "$DistDir"

[Setup]
AppId={#MyAppId}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir="$InstallerOutDir"
OutputBaseFilename=$OutputBase
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\{#MyAppExeName}
PrivilegesRequired=admin

[Languages]
Name: "chinesesimp"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#MySourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
"@

Set-Content -Path $IssPath -Value $issContent -Encoding UTF8

Write-Host "Step 2/3: generating installer script..." -ForegroundColor Green
Write-Host "  dist source : $DistName" -ForegroundColor Cyan
Write-Host "  iss file    : $IssPath" -ForegroundColor Cyan

Write-Host "Step 3/3: compiling installer with Inno Setup..." -ForegroundColor Green
& $Iscc $IssPath
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$InstallerFile = Join-Path $InstallerOutDir "$OutputBase.exe"
if (Test-Path $InstallerFile) {
    Write-Host "Installer ready: $InstallerFile" -ForegroundColor Green
} else {
    Write-Host "Installer compilation completed, check output directory: $InstallerOutDir" -ForegroundColor Yellow
}
