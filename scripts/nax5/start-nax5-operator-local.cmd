@echo off
setlocal
cd /d "%~dp0"

rem Internal operator launcher. Not a security boundary. Uses a separate QSettings namespace.
set "NAX5_API_BASE_URL=http://127.0.0.1:8000"
set "NAX5_OPERATOR_MODE=1"

if exist "chiaki.exe" (
  set "NAX5_EXE=%~dp0chiaki.exe"
) else if exist "%~dp0..\chiaki.exe" (
  set "NAX5_EXE=%~dp0..\chiaki.exe"
) else (
  echo NAX5 operator: chiaki.exe not found next to this script.
  exit /b 1
)

echo NAX5_API_BASE_URL=%NAX5_API_BASE_URL%
echo NAX5_OPERATOR_MODE=1
"%NAX5_EXE%"
endlocal
