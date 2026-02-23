@echo off
REM SwiftChatSystem Windows 构建环境准备（需管理员权限）
REM 用法: 右键“以管理员身份运行” 或 在项目根目录执行 scripts\setup-env-windows.bat
cd /d "%~dp0\.."
powershell -ExecutionPolicy Bypass -Command "Start-Process powershell -ArgumentList '-ExecutionPolicy Bypass -NoProfile -File \"%~dp0setup-env-windows.ps1\"' -Verb RunAs -Wait"
echo.
pause
exit /b %ERRORLEVEL%
