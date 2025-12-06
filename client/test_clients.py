#!/usr/bin/env python3
"""
簡易的測試腳本，啟動多個客戶端進行測試
"""

import subprocess
import time
import sys

def test_poker_clients():
    """測試多個客戶端連接"""
    print("=== Texas Hold'em Poker - Multi-Client Test ===\n")
    
    # 確認伺服器是否運行
    print("請確保伺服器已經在運行 (port 8888)")
    input("按Enter繼續...")
    
    # 玩家名稱列表
    players = ["Alice", "Bob", "Charlie", "David"]
    
    print(f"\n將啟動 {len(players)} 個客戶端...\n")
    
    # 啟動客戶端的命令（使用不同的終端窗口）
    commands = []
    
    for player in players:
        if sys.platform == "win32":
            # Windows
            cmd = f'start cmd /k "python client.py localhost 8888"'
        elif sys.platform == "darwin":
            # macOS
            cmd = f'osascript -e \'tell app "Terminal" to do script "cd {sys.path[0]} && python3 client.py localhost 8888"\''
        else:
            # Linux
            cmd = f'gnome-terminal -- bash -c "python3 client.py localhost 8888; exec bash"'
        
        commands.append(cmd)
    
    # 啟動所有客戶端
    for i, cmd in enumerate(commands):
        print(f"啟動客戶端 {i+1}/{len(commands)}: {players[i]}")
        subprocess.Popen(cmd, shell=True)
        time.sleep(0.5)  # 稍微延遲避免同時啟動
    
    print("\n所有客戶端已啟動！")
    print("\n使用指南：")
    print("1. 在每個客戶端中執行: join <玩家名稱>")
    print("   例如: join Alice")
    print("2. 在任一客戶端中執行: start")
    print("3. 遊戲開始後，按照提示進行下注")
    print("\n祝您遊戲愉快！")

if __name__ == "__main__":
    try:
        test_poker_clients()
    except KeyboardInterrupt:
        print("\n測試中斷")
    except Exception as e:
        print(f"\n錯誤: {e}")
