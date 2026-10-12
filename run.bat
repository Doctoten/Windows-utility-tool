@echo off
setlocal
cd /d "%~dp0"
if not exist dist\win_aio_optimize.exe (
  call build.bat
  if errorlevel 1 (
    pause
    exit /b 1
  )
)
start "" "%~dp0dist\win_aio_optimize.exe"
