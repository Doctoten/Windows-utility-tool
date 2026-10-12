@echo off
setlocal
cd /d "%~dp0"
where gcc >nul 2>&1
if errorlevel 1 goto cmakebuild
where windres >nul 2>&1
if errorlevel 1 goto cmakebuild
if not exist build-native mkdir build-native
if not exist dist mkdir dist
windres --codepage=65001 -I src src/app.rc -O coff -o build-native\app.res.o
if errorlevel 1 exit /b 1
gcc -std=c11 -O2 -Wall -Wextra -Werror -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0A00 -municode -mwindows src/app.c src/network.c src/backup.c src/optimizer.c src/packages.c src/protection.c src/theme.c src/system_info.c build-native\app.res.o -o dist\win_aio_optimize.exe -luser32 -lgdi32 -lcomctl32 -lcomdlg32 -lshell32 -lole32 -luuid -luxtheme -ladvapi32 -liphlpapi -lws2_32 -lwlanapi -lpowrprof -lruntimeobject -ldwmapi -loleaut32 -lwbemuuid -ldxgi -Wl,--dynamicbase,--nxcompat
if errorlevel 1 exit /b 1
echo Built: %cd%\dist\win_aio_optimize.exe
exit /b 0
:cmakebuild
where cmake >nul 2>&1
if errorlevel 1 (
  echo Install LLVM-MinGW with gcc and windres on PATH, or CMake and Visual Studio C++ Build Tools.
  exit /b 1
)
cmake -S . -B build-native -A x64
if errorlevel 1 exit /b 1
cmake --build build-native --config Release
if errorlevel 1 exit /b 1
if not exist dist mkdir dist
copy /y build-native\bin\Release\win_aio_optimize.exe dist\win_aio_optimize.exe >nul
if errorlevel 1 exit /b 1
echo Built: %cd%\dist\win_aio_optimize.exe
