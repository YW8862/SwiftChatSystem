@echo off
setlocal

set SCRIPT_DIR=%~dp0
set PS_SCRIPT=%SCRIPT_DIR%package-client-installer.ps1

powershell -NoProfile -ExecutionPolicy Bypass -File "%PS_SCRIPT%" %*
set EXIT_CODE=%ERRORLEVEL%

if %EXIT_CODE% neq 0 (
  echo.
  echo Installer packaging failed with code %EXIT_CODE%.
  exit /b %EXIT_CODE%
)

echo.
echo Installer packaging completed.
endlocal
