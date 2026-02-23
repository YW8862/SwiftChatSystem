@echo off
chcp 65001 >nul
REM SwiftChatSystem Windows 构建环境准备（需管理员权限）
REM 必须: 右键本文件 -> 以管理员身份运行（否则安装会失败，且本窗口会保留便于查看输出）
set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR:~0,-1%\.."
if not exist "%PROJECT_ROOT%\client" (
    echo 错误: 未找到项目根目录（缺少 client 文件夹）。
    echo 请将本 bat 放在「项目\scripts\」目录下。
    echo.
    pause
    exit /b 1
)
cd /d "%PROJECT_ROOT%"
echo 若未以管理员身份运行，PowerShell 会提示权限错误，请关闭后右键本文件 -^> 以管理员身份运行。
echo.
powershell -ExecutionPolicy Bypass -File "%SCRIPT_DIR%setup-env-windows.ps1"
echo.
pause
exit /b %ERRORLEVEL%
