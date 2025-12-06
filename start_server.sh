#!/bin/bash
# Linux/Mac 啟動腳本

echo "===================================="
echo "Texas Hold'em Poker Server Launcher"
echo "===================================="
echo

# 尋找可執行檔
if [ -f "build/TexasHoldemServer" ]; then
    echo "Starting server from CMake build..."
    ./build/TexasHoldemServer 8888
elif [ -f "TexasHoldemServer" ]; then
    echo "Starting server..."
    ./TexasHoldemServer 8888
else
    echo "Error: Server executable not found!"
    echo "Please compile the project first:"
    echo
    echo "Using CMake:"
    echo "  mkdir build"
    echo "  cd build"
    echo "  cmake .."
    echo "  make"
    echo
    exit 1
fi
