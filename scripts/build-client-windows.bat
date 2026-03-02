@echo off
$env:VCPKG_ROOT="E:\vcpkg"
chcp 65001 >nul
REM SwiftChatSystem Windows client build launcher
REM Usage: double-click, or run from scripts folder
set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR:~0,-1%\.."
if not exist "%PROJECT_ROOT%\client" (
    echo ERROR: project root not found ^(missing client folder^).
    echo Please place this bat under "project\scripts\", e.g.
    echo   D:\SwiftChatSystem\scripts\build-client-windows.bat
    echo Current script dir: %SCRIPT_DIR%
    echo.
    pause
    exit /b 1
)
cd /d "%PROJECT_ROOT%"
powershell -ExecutionPolicy Bypass -File "%SCRIPT_DIR%build-client-windows.ps1"
echo.
pause
exit /b %ERRORLEVEL%
