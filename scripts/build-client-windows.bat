@echo off
chcp 65001 >nul
REM SwiftChatSystem Windows 客户端构建
REM 用法: 双击运行，或进入项目 scripts 目录后执行 build-client-windows.bat
set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR:~0,-1%\.."
if not exist "%PROJECT_ROOT%\client" (
    echo 错误: 未找到项目根目录（缺少 client 文件夹）。
    echo 请将本 bat 放在「项目\scripts\」目录下，例如: D:\SwiftChatSystem\scripts\build-client-windows.bat
    echo 当前脚本所在: %SCRIPT_DIR%
    echo.
    pause
    exit /b 1
)
cd /d "%PROJECT_ROOT%"
powershell -ExecutionPolicy Bypass -File "%SCRIPT_DIR%build-client-windows.ps1"
echo.
pause
exit /b %ERRORLEVEL%
