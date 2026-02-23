@echo off
REM SwiftChatSystem Windows 客户端构建
REM 用法: 在项目根目录执行 scripts\build-client-windows.bat
cd /d "%~dp0\.."

powershell -ExecutionPolicy Bypass -File "%~dp0build-client-windows.ps1"
echo.
pause
exit /b %ERRORLEVEL%
