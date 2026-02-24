@echo off
chcp 65001 >nul
REM SwiftChatSystem Windows 客户端一键打包（基于 vcpkg Qt）
REM 用法: 在项目根目录的 scripts 目录下双击本文件，或在命令行中执行：
REM   scripts\package-client-vcpkg.bat

set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR:~0,-1%\.."

if not exist "%PROJECT_ROOT%\client" (
    echo 错误: 未找到项目根目录（缺少 client 文件夹）。
    echo 请将本 bat 放在「項目\scripts\」目录下，例如: D:\SwiftChatSystem\scripts\package-client-vcpkg.bat
    echo 當前腳本所在: %SCRIPT_DIR%
    echo.
    pause
    exit /b 1
)

cd /d "%PROJECT_ROOT%"
echo 项目根目录: %CD%
echo.
powershell -ExecutionPolicy Bypass -File "%SCRIPT_DIR%package-client-vcpkg-simple.ps1"
echo.
pause
exit /b %ERRORLEVEL%

