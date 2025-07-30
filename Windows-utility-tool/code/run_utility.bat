@echo off
chcp 65001
echo ===============================================
echo    Tiện ích hỗ trợ cài Windows - Win dao
echo    Windows Installation Support Utility
echo ===============================================
echo.

REM Check if Python is installed
python --version >nul 2>&1
if errorlevel 1 (
    echo [LỖI] Python chưa được cài đặt!
    echo [ERROR] Python is not installed!
    echo Vui lòng cài đặt Python từ: https://python.org
    echo Please install Python from: https://python.org
    pause
    exit /b 1
)

echo [INFO] Đang khởi động ứng dụng...
echo [INFO] Starting application...
echo.

REM Run the Python application
python windows_utility_tool.py

REM Check if the application ran successfully
if errorlevel 1 (
    echo.
    echo [LỖI] Có lỗi xảy ra khi chạy ứng dụng!
    echo [ERROR] An error occurred while running the application!
    pause
) else (
    echo.
    echo [INFO] Ứng dụng đã đóng thành công.
    echo [INFO] Application closed successfully.
)

echo.
echo Nhấn phím bất kỳ để thoát...
echo Press any key to exit...
pause >nul 