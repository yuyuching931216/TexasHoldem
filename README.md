# Texas Hold'em Online Poker

一個基於C++和Python的線上德州撲克遊戲系統。

## ⚡ 最新更新

**v1.1 - 死鎖問題修復** (2024)
- ✅ 修復了 Room.cpp 中的資源死鎖問題
- ✅ 優化了互斥鎖策略，提高線程安全性
- ✅ 改進了性能和響應時間
- 詳情請參閱 [FIX_SUMMARY.md](FIX_SUMMARY.md)

## 功能特點

- 🎮 **單房間多人遊戲**: 支持2-10名玩家
- 🔒 **安全記憶體管理**: 使用現代C++智能指針
- 🌐 **網路通訊**: 基於Boost.Asio的高效網路框架
- 🐍 **跨平台客戶端**: Python實現的簡單CLI客戶端
- ⚡ **異步I/O**: 高效的非阻塞網路通訊
- 🎲 **完整遊戲邏輯**: 支持德州撲克全部規則
- 🛡️ **線程安全**: 無死鎖風險的併發處理

## 系統架構

### 伺服器端 (C++)
- `Server.cpp/h`: 網路伺服器，管理連接
- `Session.cpp/h`: 客戶端會話管理
- `Room.cpp/h`: 遊戲房間管理
- `Game.cpp/h`: 遊戲邏輯核心
- `Player.cpp/h`: 玩家狀態管理
- `Card.cpp/h`: 撲克牌和牌組
- `HandEvaluator.cpp/h`: 手牌評估器

### 客戶端 (Python)
- `client.py`: CLI客戶端，提供互動式命令界面

## 編譯需求

### C++ 伺服器
- C++17 或更高版本
- Boost 1.70+ (需要 Boost.Asio)
- CMake 3.10+ 或 Visual Studio 2019+

### Python 客戶端
- Python 3.6+
- 標準庫（無需額外依賴）

## 編譯與運行

### 編譯伺服器 (Windows + Visual Studio)

```bash
# 使用Visual Studio打開 TexasHoldem.sln
# 確保已安裝 Boost 庫
# 編譯並運行 Release 或 Debug 配置
```

### 編譯伺服器 (CMake)

如果想使用CMake，創建 `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.10)
project(TexasHoldemServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 找到 Boost
find_package(Boost 1.70 REQUIRED COMPONENTS system)

# 添加源文件
add_executable(TexasHoldemServer
    server/main.cpp
    server/Server.cpp
    server/Session.cpp
    server/Room.cpp
    server/Game.cpp
    server/Player.cpp
    server/Card.cpp
    server/HandEvaluator.cpp
)

# 鏈接 Boost
target_include_directories(TexasHoldemServer PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(TexasHoldemServer ${Boost_LIBRARIES})

# Windows 需要額外的網路庫
if(WIN32)
    target_link_libraries(TexasHoldemServer ws2_32)
endif()
```

然後編譯：

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### 運行伺服器

```bash
# Windows
.\TexasHoldemServer.exe [port]

# Linux/Mac
./TexasHoldemServer [port]

# 預設端口: 8888
```

### 運行客戶端

```bash
# 進入客戶端目錄
cd client

# 運行客戶端
python client.py [host] [port]

# 範例：連接到本地伺服器
python client.py localhost 8888
```

## 使用說明

### 客戶端命令

連接到伺服器後，可以使用以下命令：

```
join <name> [buyin]  - 加入遊戲 (例: join Alice 1000)
start                - 開始遊戲 (需要至少2名玩家)
fold                 - 棄牌
check                - 過牌
call                 - 跟注
raise <amount>       - 加注 (例: raise 100)
allin                - 全下
status               - 查看房間狀態
quit                 - 離開遊戲
help                 - 顯示幫助信息
```

### 遊戲流程範例

1. **啟動伺服器**：
   ```bash
   ./TexasHoldemServer 8888
   ```

2. **客戶端1加入**：
   ```bash
   python client.py
   > join Alice 1000
   ✓ Joined room as Alice
   ```

3. **客戶端2加入**：
   ```bash
   python client.py
   > join Bob 1000
   ✓ Joined room as Bob
   ```

4. **開始遊戲**：
   ```bash
   > start
   🎮 Game is starting...
   ```

5. **進行下注**：
   ```bash
   > call
   → Player 1: CALL $20
   
   > raise 50
   → Player 2: RAISE $50
   
   > fold
   → Player 1: FOLD
   ```

## 通訊協議

伺服器與客戶端使用基於文本的協議通訊，每條消息以換行符結尾。

### 客戶端 → 伺服器

```
JOIN <playerName> [buyIn]      - 加入房間
START                          - 開始遊戲
ACTION <action> [amount]       - 執行動作
  action: FOLD, CHECK, CALL, RAISE, ALL_IN
STATUS                         - 查詢狀態
QUIT                           - 離開
```

### 伺服器 → 客戶端

```
OK|<message>                   - 操作成功
ERROR|<message>                - 錯誤消息
JOINED|<id>|<name>|<chips>     - 玩家加入
LEFT|<id>                      - 玩家離開
PLAYERS|<playerList>           - 玩家列表
GAME_START|<message>           - 遊戲開始
STATE|<gameState>              - 遊戲狀態更新
ACTION|<id>|<action>|[amount]  - 玩家動作
STATUS|<roomStatus>            - 房間狀態
BYE|<message>                  - 再見消息
```

## 安全性和記憶體管理

本項目使用現代C++特性確保安全性：

- ✅ **智能指針**: 使用 `std::shared_ptr` 和 `std::unique_ptr` 管理動態記憶體
- ✅ **RAII**: 資源獲取即初始化，自動管理資源生命週期
- ✅ **移動語義**: 使用 `std::move` 避免不必要的複製
- ✅ **異常安全**: 完整的錯誤處理機制
- ✅ **線程安全**: 使用 `std::mutex` 保護共享資源

## 擴展功能建議

目前版本實現了基本功能，可以考慮添加：

- [ ] 多房間支持
- [ ] 玩家身份驗證
- [ ] 遊戲歷史記錄
- [ ] 觀察者模式
- [ ] 聊天功能
- [ ] Web界面客戶端
- [ ] 數據庫持久化
- [ ] 排行榜系統
- [ ] 錦標賽模式
- [ ] AI玩家

## 故障排除

### Boost庫找不到

確保已安裝Boost並設置正確的環境變量：

**Windows**: 
- 下載 Boost 並解壓
- 設置 `BOOST_ROOT` 環境變量
- 或在Visual Studio中設置包含目錄

**Linux**:
```bash
sudo apt-get install libboost-all-dev
```

**Mac**:
```bash
brew install boost
```

### 編譯錯誤

確保使用C++17或更高版本：
```cmake
set(CMAKE_CXX_STANDARD 17)
```

### 連接失敗

- 檢查防火牆設置
- 確保端口未被占用
- 確認伺服器正在運行

## 授權

此項目僅供學習和教育目的使用。

## 貢獻

歡迎提交問題和改進建議！

## 聯繫方式

如有問題，請在GitHub上開issue。
