#!/bin/bash
# Linux/Mac 客戶端啟動腳本

echo "===================================="
echo "Texas Hold'em Poker Client Launcher"
echo "===================================="
echo

cd client

# 檢查 Python 是否安裝
if ! command -v python3 &> /dev/null; then
    echo "Error: Python 3 is not installed"
    echo "Please install Python 3.6 or higher"
    exit 1
fi

# 詢問伺服器地址
read -p "Enter server address (default: localhost): " SERVER_HOST
SERVER_HOST=${SERVER_HOST:-localhost}

read -p "Enter server port (default: 8888): " SERVER_PORT
SERVER_PORT=${SERVER_PORT:-8888}

echo
echo "Connecting to $SERVER_HOST:$SERVER_PORT..."
echo

python3 client.py $SERVER_HOST $SERVER_PORT
