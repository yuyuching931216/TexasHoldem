# 快速入門指南

## 5分鐘快速開始

### 步驟 1: 編譯伺服器

#### 使用 Visual Studio (Windows)
1. 打開 `TexasHoldem.sln`
2. 選擇 **Release** 配置
3. 按 `F7` 或點擊 **Build > Build Solution**
4. 等待編譯完成

#### 使用 CMake (跨平台)
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### 步驟 2: 啟動伺服器

#### Windows
```bash
# 使用啟動腳本
start_server.bat

# 或直接運行
x64\Release\TexasHoldemServer.exe
```

#### Linux/Mac
```bash
# 使用啟動腳本
chmod +x start_server.sh
./start_server.sh

# 或直接運行
./build/TexasHoldemServer
```

伺服器啟動後會顯示：
```
=== Texas Hold'em Poker Server ===
Starting server on port 8888...
Texas Hold'em Server started on port 8888
Waiting for players...
Server is running. Press Ctrl+C to stop.
```

### 步驟 3: 連接客戶端

#### 方法1: 使用啟動腳本

**Windows:**
```bash
start_client.bat
```

**Linux/Mac:**
```bash
chmod +x start_client.sh
./start_client.sh
```

#### 方法2: 直接運行

```bash
cd client
python client.py localhost 8888
```

### 步驟 4: 開始遊戲

在客戶端中輸入以下命令：

1. **加入遊戲** (至少需要2名玩家)
```
> join Alice 1000
✓ Joined room as Alice
```

2. **在另一個客戶端也加入**
```
> join Bob 1000
✓ Joined room as Bob
```

3. **開始遊戲** (任一玩家)
```
> start
🎮 Game is starting...
```

4. **進行下注** (根據遊戲狀態)
```
> call      # 跟注
> raise 50  # 加注
> fold      # 棄牌
> check     # 過牌
> allin     # 全下
```

## 常用命令

```
join <name> [buyin]  - 加入遊戲 (默認買入1000)
start                - 開始遊戲
fold                 - 棄牌
check                - 過牌
call                 - 跟注
raise <amount>       - 加注
allin                - 全下
status               - 查看房間狀態
quit                 - 退出
help                 - 顯示幫助
```

## 遊戲流程示例

### 完整的一局遊戲

**客戶端 1 (Alice):**
```
> join Alice 1000
✓ Joined room as Alice

> start
🎮 Game is starting...

→ Player Alice: CALL $20

> call
→ Player 1: CALL $20
```

**客戶端 2 (Bob):**
```
> join Bob 1000
✓ Joined room as Bob
→ Player Alice (ID: 1) joined with $1000

> raise 50
→ Player 2: RAISE $50
```

## 測試多個客戶端

你可以使用測試腳本同時啟動多個客戶端：

```bash
cd client
python test_clients.py
```

這會在不同的終端窗口中啟動多個客戶端，方便測試。

## 故障排除

### 無法連接到伺服器
- 確認伺服器正在運行
- 檢查防火牆設置
- 確認端口 8888 未被占用

### 編譯錯誤
- 確保已安裝 Boost 庫 (1.70+)
- 確保使用 C++17 或更高版本
- Windows: 設置 BOOST_ROOT 環境變量

### Python 客戶端無法運行
- 確保安裝了 Python 3.6+
- 檢查 Python 是否在 PATH 中

## 進階使用

### 自定義端口

**伺服器:**
```bash
TexasHoldemServer.exe 9999
```

**客戶端:**
```bash
python client.py localhost 9999
```

### 遠程連接

**伺服器:**
```bash
# 伺服器會監聽 0.0.0.0:8888，接受任何IP的連接
TexasHoldemServer.exe 8888
```

**客戶端:**
```bash
python client.py 192.168.1.100 8888
```

## 下一步

- 閱讀完整的 [README.md](README.md) 了解詳細信息
- 查看通訊協議了解如何開發自己的客戶端
- 修改 `config.ini` 自定義遊戲規則

## 需要幫助？

如果遇到問題：
1. 查看伺服器終端的錯誤消息
2. 檢查客戶端的錯誤提示
3. 在 GitHub 上提交 issue
4. 參考 README.md 中的故障排除部分

祝你遊戲愉快！ 🎮♠♥♣♦
