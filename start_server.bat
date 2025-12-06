@echo off
REM Windows 啟動腳本

echo ====================================
echo Texas Hold'em Poker Server Launcher
echo ====================================
echo.

REM 檢查是否有編譯好的執行檔
if exist "x64\Release\TexasHoldemServer.exe" (
    echo Starting server from Release build...
    x64\Release\TexasHoldemServer.exe 8888
) else if exist "x64\Debug\TexasHoldemServer.exe" (
    echo Starting server from Debug build...
    x64\Debug\TexasHoldemServer.exe 8888
) else if exist "build\Release\TexasHoldemServer.exe" (
    echo Starting server from CMake Release build...
    build\Release\TexasHoldemServer.exe 8888
) else if exist "build\Debug\TexasHoldemServer.exe" (
    echo Starting server from CMake Debug build...
    build\Debug\TexasHoldemServer.exe 8888
) else (
    echo Error: Server executable not found!
    echo Please compile the project first.
    echo.
    echo Using Visual Studio:
    echo   1. Open TexasHoldem.sln
    echo   2. Build the solution (F7)
    echo.
    echo Using CMake:
    echo   mkdir build
    echo   cd build
    echo   cmake ..
    echo   cmake --build . --config Release
    echo.
    pause
    exit /b 1
)

pause
