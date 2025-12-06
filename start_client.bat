@echo off
REM Windows 客戶端啟動腳本

echo ====================================
echo Texas Hold'em Poker Client Launcher
echo ====================================
echo.

cd client

REM 檢查 Python 是否安裝
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: Python is not installed or not in PATH
    echo Please install Python 3.6 or higher
    pause
    exit /b 1
)

echo Enter server address (default: localhost):
set /p SERVER_HOST=
if "%SERVER_HOST%"=="" set SERVER_HOST=localhost

echo Enter server port (default: 8888):
set /p SERVER_PORT=
if "%SERVER_PORT%"=="" set SERVER_PORT=8888

echo.
echo Connecting to %SERVER_HOST%:%SERVER_PORT%...
echo.

python client.py %SERVER_HOST% %SERVER_PORT%

pause
