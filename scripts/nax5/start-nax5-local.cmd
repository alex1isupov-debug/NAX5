@echo off
setlocal
cd /d "%~dp0"

rem Dev helper only. Production chiaki.exe uses https://cloudgta6.com without this script.
set "NAX5_API_BASE_URL=http://127.0.0.1:8000"

if exist "chiaki.exe" (
  set "NAX5_EXE=%~dp0chiaki.exe"
) else if exist "%~dp0..\chiaki.exe" (
  set "NAX5_EXE=%~dp0..\chiaki.exe"
) else (
  echo NAX5: chiaki.exe not found next to this script.
  exit /b 1
)

powershell -NoProfile -Command "try { Invoke-WebRequest -UseBasicParsing -TimeoutSec 2 http://127.0.0.1:8000/health/live | Out-Null } catch { exit 1 }"
if errorlevel 1 (
  echo NAX5: local backend is not up at http://127.0.0.1:8000
  echo Start Django first, then run this helper again.
  exit /b 1
)

echo NAX5_API_BASE_URL=%NAX5_API_BASE_URL%
rem Do not use Windows start to launch chiaki.exe; that drops the environment.
"%NAX5_EXE%"
endlocal
