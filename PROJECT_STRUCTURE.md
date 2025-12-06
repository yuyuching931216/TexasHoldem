# 專案結構說明

```
TexasHoldem/
│
├── server/                      # C++ 伺服器端源碼
│   ├── main.cpp                 # 主程序入口
│   ├── Server.h / .cpp          # 網路伺服器類
│   ├── Session.h / .cpp         # 客戶端會話管理
│   ├── Room.h / .cpp            # 遊戲房間管理
│   ├── Game.h / .cpp            # 遊戲邏輯核心
│   ├── Player.h / .cpp          # 玩家類
│   ├── Card.h / .cpp            # 撲克牌和牌組
│   └── HandEvaluator.h / .cpp   # 手牌評估器
│
├── client/                      # Python 客戶端
│   ├── client.py                # CLI 客戶端
│   └── test_clients.py          # 多客戶端測試腳本
│
├── build/                       # CMake 構建目錄 (自動生成)
│   └── ...
│
├── x64/                         # Visual Studio 輸出 (自動生成)
│   ├── Debug/
│   └── Release/
│
├── docs/                        # 文檔 (可選創建)
│   ├── protocol.md              # 通訊協議文檔
│   ├── architecture.md          # 架構設計文檔
│   └── api.md                   # API 文檔
│
├── CMakeLists.txt               # CMake 配置文件
├── TexasHoldem.sln              # Visual Studio 解決方案
├── TexasHoldem.vcxproj          # Visual Studio 專案文件
├── TexasHoldem.vcxproj.filters  # Visual Studio 篩選器
│
├── README.md                    # 專案說明
├── QUICKSTART.md                # 快速入門指南
├── INSTALLATION.md              # 依賴安裝指南
├── VISUAL_STUDIO_SETUP.md       # Visual Studio 配置
├── TESTING.md                   # 測試指南
├── PROJECT_STRUCTURE.md         # 本文件
│
├── config.ini                   # 配置文件範例
├── .gitignore                   # Git 忽略文件
│
├── start_server.bat             # Windows 伺服器啟動腳本
├── start_server.sh              # Linux/Mac 伺服器啟動腳本
├── start_client.bat             # Windows 客戶端啟動腳本
├── start_client.sh              # Linux/Mac 客戶端啟動腳本
│
└── update_project.py            # 專案文件更新腳本

```

## 核心模塊說明

### 伺服器端 (C++)

#### 網路層
- **Server**: 管理 TCP 連接，接受新的客戶端
- **Session**: 處理單個客戶端的通訊，解析命令，發送回應

#### 遊戲邏輯層
- **Room**: 管理一個遊戲房間，協調玩家和遊戲狀態
- **Game**: 實現德州撲克的核心遊戲邏輯
- **Player**: 玩家狀態管理，包括籌碼、動作、手牌等

#### 工具層
- **Card**: 撲克牌表示
- **Deck**: 牌組管理，洗牌、發牌
- **HandEvaluator**: 評估手牌大小，比較勝負

### 客戶端 (Python)

- **PokerClient**: 主要客戶端類
  - 連接管理
  - 消息發送/接收
  - 命令解析
  - UI 顯示

## 數據流

```
┌─────────────┐                 ┌─────────────┐
│   Client 1  │                 │   Client 2  │
│  (Python)   │                 │  (Python)   │
└──────┬──────┘                 └──────┬──────┘
       │                               │
       │ JOIN Alice                    │ JOIN Bob
       │                               │
       ├───────────┐         ┌─────────┤
       │           │         │         │
       │           ▼         ▼         │
       │      ┌─────────────────┐      │
       │      │     Server      │      │
       │      │  (Boost.Asio)   │      │
       │      └────────┬────────┘      │
       │               │               │
       │               ▼               │
       │      ┌─────────────────┐      │
       │      │      Room       │      │
       │      │  (Game Logic)   │      │
       │      └────────┬────────┘      │
       │               │               │
       │      ┌────────┴────────┐      │
       │      ▼                 ▼      │
       │  ┌─────────┐      ┌─────────┐│
       │  │Session 1│      │Session 2││
       │  └────┬────┘      └────┬────┘│
       │       │                │     │
       ◄───────┘                └─────►
     JOINED|1|Alice         JOINED|2|Bob
```

## 類關係圖

```
┌──────────────────────────────────────────┐
│                 Server                   │
│  - acceptor_: tcp::acceptor              │
│  - room_: shared_ptr<Room>               │
└────────────────┬─────────────────────────┘
                 │ has-a
                 ▼
┌──────────────────────────────────────────┐
│                  Room                    │
│  - sessions_: map<int, Session*>         │
│  - game_: unique_ptr<PokerGame>          │
└────────────┬─────────────┬───────────────┘
             │ has-many    │ has-a
             ▼             ▼
    ┌────────────┐   ┌──────────────┐
    │  Session   │   │  PokerGame   │
    │            │   │              │
    └────────────┘   └──────┬───────┘
                            │ has-many
                            ▼
                     ┌─────────────┐
                     │   Player    │
                     │             │
                     └──────┬──────┘
                            │ has-many
                            ▼
                     ┌─────────────┐
                     │    Card     │
                     │             │
                     └─────────────┘
```

## 線程模型

```
主線程 (Main Thread)
│
├─> Boost.Asio io_context.run()
│   │
│   ├─> Accept Thread (處理新連接)
│   │
│   ├─> Session 1 Thread (處理客戶端1)
│   │   └─> async_read_some
│   │       └─> async_write
│   │
│   ├─> Session 2 Thread (處理客戶端2)
│   │   └─> async_read_some
│   │       └─> async_write
│   │
│   └─> ... (更多會話)
│
└─> Room (共享資源，使用 mutex 保護)
    └─> Game (遊戲邏輯)
```

## 通訊協議層次

```
┌───────────────────────────────────┐
│   Application Layer (命令層)      │
│   JOIN, START, ACTION, etc.       │
└───────────────┬───────────────────┘
                │
┌───────────────▼───────────────────┐
│   Message Layer (消息層)          │
│   文本協議，換行符分隔            │
└───────────────┬───────────────────┘
                │
┌───────────────▼───────────────────┐
│   Transport Layer (傳輸層)        │
│   TCP/IP (Boost.Asio)             │
└───────────────────────────────────┘
```

## 記憶體管理策略

### 智能指針使用

```cpp
// Server 持有 Room
std::shared_ptr<Room> room_;

// Room 持有 Sessions
std::map<int, std::shared_ptr<Session>> sessions_;

// Room 持有 Game
std::unique_ptr<PokerGame> game_;

// Game 持有 Players (value semantics)
std::vector<Player> players_;
```

### 生命週期

1. **Server**: 程序運行期間一直存在
2. **Room**: 由 Server 創建，Server 關閉時釋放
3. **Session**: 客戶端連接時創建，斷開時自動釋放
4. **Game**: Room 創建時初始化，可重置重用
5. **Player**: 加入遊戲時創建，離開時銷毀

## 配置文件結構

```ini
# config.ini 結構

[Server]
port=8888
host=0.0.0.0

[Room]
max_players=10
min_players=2
default_buy_in=1000

[Game]
small_blind=10
big_blind=20
min_raise=20

[Timeout]
action_timeout=30
game_start_delay=5
```

## 擴展點

### 添加新功能時的建議位置

1. **新的遊戲規則**: 修改 `Game.cpp`
2. **新的玩家動作**: 修改 `Player.h/cpp` 和 `Room.cpp`
3. **新的命令**: 修改 `Session.cpp` 的 `processMessage`
4. **多房間支持**: 在 `Server` 中維護多個 `Room`
5. **身份驗證**: 在 `Session` 中添加驗證邏輯
6. **數據持久化**: 添加 `Database` 模塊
7. **聊天功能**: 在 `Room` 中添加 `broadcast` 類型消息

## 測試結構

```
tests/                       # 測試目錄 (可選創建)
│
├── unit/                    # 單元測試
│   ├── test_card.cpp
│   ├── test_deck.cpp
│   ├── test_hand_evaluator.cpp
│   └── test_player.cpp
│
├── integration/             # 集成測試
│   ├── test_game.cpp
│   └── test_room.cpp
│
└── e2e/                     # 端到端測試
    └── test_full_game.py
```

## 日誌結構 (未來可添加)

```
logs/
├── server_YYYYMMDD.log      # 伺服器日誌
├── game_YYYYMMDD.log        # 遊戲日誌
└── error_YYYYMMDD.log       # 錯誤日誌
```

## 構建產物

### CMake 構建
```
build/
├── CMakeCache.txt
├── CMakeFiles/
├── Makefile
└── TexasHoldemServer        # 可執行文件
```

### Visual Studio 構建
```
x64/
├── Debug/
│   ├── TexasHoldem.exe
│   └── *.obj, *.pdb
└── Release/
    ├── TexasHoldem.exe
    └── *.obj
```

## 版本控制

建議使用 Git，重要文件：
- ✅ 包含: 所有源代碼、頭文件、CMakeLists.txt
- ✅ 包含: README、文檔
- ❌ 排除: build/、x64/、*.obj、*.exe
- ❌ 排除: .vs/、*.user、*.suo

## 文檔維護

| 文檔 | 更新時機 |
|------|---------|
| README.md | 功能變更、重要更新 |
| QUICKSTART.md | 使用流程變更 |
| PROJECT_STRUCTURE.md | 結構變更 |
| 代碼註釋 | 代碼修改時同步更新 |

## 未來規劃目錄

```
TexasHoldem/
├── assets/                  # 資源文件（未來）
│   ├── images/
│   └── sounds/
│
├── web/                     # Web 客戶端（未來）
│   ├── index.html
│   ├── js/
│   └── css/
│
└── database/                # 數據庫（未來）
    ├── schema.sql
    └── migrations/
```

## 相關資源

- [Boost.Asio 示例](https://www.boost.org/doc/libs/release/doc/html/boost_asio/examples.html)
- [德州撲克規則](https://en.wikipedia.org/wiki/Texas_hold_%27em)
- [TCP 協議設計最佳實踐](https://datatracker.ietf.org/doc/html/rfc793)
