# JSON 通訊協議 v2.0

## 概述

伺服器與客戶端之間使用 JSON 格式進行通訊。每條消息以換行符 `\n` 結尾。

---

## 客戶端 → 伺服器

### 文本命令（向後兼容）

```
JOIN <name> [buyin]     # 加入遊戲
START                   # 開始遊戲
ACTION <action> [amount]# 執行動作
STATUS                  # 查詢房間狀態
GAMESTATE               # 查詢遊戲狀態
PLAYERS                 # 查詢玩家列表
QUIT                    # 離開
```

### JSON 命令

```json
{"command": "JOIN", "name": "Alice", "buy_in": 1000}
{"command": "START"}
{"action": "FOLD"}
{"action": "CHECK"}
{"action": "CALL"}
{"action": "RAISE", "amount": 100}
{"action": "ALL_IN"}
{"command": "STATUS"}
{"command": "GAMESTATE"}
{"command": "QUIT"}
```

---

## 伺服器 → 客戶端

### 所有消息都是 JSON 格式

#### OK - 操作成功
```json
{"type": "OK", "message": "Joined room as Alice"}
```

#### ERROR - 錯誤
```json
{"type": "ERROR", "message": "Room is full"}
```

#### JOINED - 玩家加入
```json
{
    "type": "JOINED",
    "player_id": 1,
    "name": "Alice",
    "chips": 1000
}
```

#### LEFT - 玩家離開
```json
{"type": "LEFT", "player_id": 1}
```

#### PLAYERS - 玩家列表
```json
{
    "type": "PLAYERS",
    "players": [
        {
            "id": 1,
            "name": "Alice",
            "chips": 1000,
            "current_bet": 0,
            "state": 0,
            "is_dealer": true,
            "is_small_blind": false,
            "is_big_blind": false
        }
    ]
}
```

#### GAME_START - 遊戲開始
```json
{
    "type": "GAME_START",
    "message": "Game is starting...",
    "stage": "preflop"
}
```

#### HOLE_CARDS - 玩家手牌（只發給該玩家）
```json
{
    "type": "HOLE_CARDS",
    "cards": [
        {"rank": "A", "suit": "hearts", "short": "Ah"},
        {"rank": "K", "suit": "spades", "short": "Ks"}
    ]
}
```

#### GAME_STATE - 遊戲狀態
```json
{
    "type": "GAME_STATE",
    "pot": 150,
    "current_bet": 50,
    "stage": "flop",
    "current_player": 2,
    "community_cards": [
        {"rank": "2", "suit": "hearts", "short": "2h"},
        {"rank": "5", "suit": "diamonds", "short": "5d"},
        {"rank": "K", "suit": "clubs", "short": "Kc"}
    ],
    "players": [
        {
            "id": 1,
            "name": "Alice",
            "chips": 950,
            "current_bet": 50,
            "state": 0,
            "is_dealer": true,
            "is_small_blind": false,
            "is_big_blind": false
        },
        {
            "id": 2,
            "name": "Bob",
            "chips": 900,
            "current_bet": 50,
            "state": 0,
            "is_dealer": false,
            "is_small_blind": true,
            "is_big_blind": false
        }
    ]
}
```

#### ROOM_STATE - 房間狀態
```json
{
    "type": "ROOM_STATE",
    "room_id": 1,
    "player_count": 3,
    "max_players": 10,
    "game_in_progress": true,
    "stage": "flop"
}
```

#### YOUR_TURN - 輪到你行動
```json
{
    "type": "YOUR_TURN",
    "player_id": 1,
    "to_call": 30,
    "current_bet": 50,
    "pot": 80,
    "min_raise": 70
}
```

#### CURRENT_PLAYER - 當前行動玩家
```json
{"type": "CURRENT_PLAYER", "player_id": 2}
```

#### ACTION - 玩家動作
```json
{"type": "ACTION", "player_id": 1, "action": "RAISE", "amount": 100}
{"type": "ACTION", "player_id": 2, "action": "CALL", "amount": 50}
{"type": "ACTION", "player_id": 3, "action": "FOLD"}
{"type": "ACTION", "player_id": 4, "action": "CHECK"}
```

#### STAGE_CHANGE - 階段變更
```json
{"type": "STAGE_CHANGE", "stage": "turn"}
```

階段值：
- `waiting` - 等待開始
- `preflop` - 翻牌前
- `flop` - 翻牌
- `turn` - 轉牌
- `river` - 河牌
- `showdown` - 攤牌

#### SHOWDOWN - 攤牌
```json
{
    "type": "SHOWDOWN",
    "players": [
        {
            "id": 1,
            "name": "Alice",
            "chips": 1200,
            "hand": [
                {"rank": "A", "suit": "hearts", "short": "Ah"},
                {"rank": "A", "suit": "spades", "short": "As"}
            ]
        }
    ]
}
```

#### GAME_END - 遊戲結束
```json
{"type": "GAME_END", "message": "Hand complete"}
```

#### BYE - 再見
```json
{"type": "BYE", "message": "Goodbye"}
```

---

## 玩家狀態代碼

| 代碼 | 狀態 |
|------|------|
| 0 | Active（活躍）|
| 1 | Folded（已棄牌）|
| 2 | All-in（全下）|
| 3 | Disconnected（斷線）|
| 4 | Waiting（等待）|

---

## 牌面格式

### 完整格式
```json
{
    "rank": "A",      // 2-9, 10, J, Q, K, A
    "suit": "hearts", // hearts, diamonds, clubs, spades
    "short": "Ah"     // 短格式: 2h, Td, Js, Qc, Kd, As
}
```

### 短格式
- Rank: `2`, `3`, `4`, `5`, `6`, `7`, `8`, `9`, `T`, `J`, `Q`, `K`, `A`
- Suit: `h` (hearts), `d` (diamonds), `c` (clubs), `s` (spades)
- 例如: `Ah` = Ace of Hearts, `Td` = 10 of Diamonds

---

## 遊戲流程

### 1. 連接和加入
```
Client -> Server: JOIN Alice 1000
Server -> All:    {"type": "JOINED", "player_id": 1, "name": "Alice", "chips": 1000}
Server -> Client: {"type": "PLAYERS", "players": [...]}
```

### 2. 開始遊戲
```
Client -> Server: START
Server -> All:    {"type": "GAME_START", "stage": "preflop"}
Server -> Alice:  {"type": "HOLE_CARDS", "cards": [...]}
Server -> Bob:    {"type": "HOLE_CARDS", "cards": [...]}
Server -> All:    {"type": "GAME_STATE", ...}
Server -> Player: {"type": "YOUR_TURN", ...}
```

### 3. 玩家行動
```
Client -> Server: ACTION CALL
Server -> All:    {"type": "ACTION", "player_id": 1, "action": "CALL", "amount": 20}
Server -> All:    {"type": "GAME_STATE", ...}
Server -> Next:   {"type": "YOUR_TURN", ...}
```

### 4. 階段變更
```
Server -> All:    {"type": "STAGE_CHANGE", "stage": "flop"}
Server -> All:    {"type": "GAME_STATE", ...}  // 包含公共牌
```

### 5. 攤牌和結束
```
Server -> All:    {"type": "SHOWDOWN", "players": [...]}
Server -> All:    {"type": "GAME_END", "message": "Hand complete"}
```

---

## AI 客戶端使用

### 啟動 AI
```bash
python pokerai.py localhost 8888 AI_Player1
```

### AI 決策流程

1. 收到 `HOLE_CARDS` 保存手牌
2. 收到 `GAME_STATE` 更新狀態
3. 收到 `YOUR_TURN` 時做決策
4. 根據勝率發送 `ACTION`

### 勝率計算

AI 使用蒙特卡洛模擬估算勝率：
- 500 次模擬
- 考慮對手數量
- 根據階段調整門檻

---

## 錯誤處理

### 常見錯誤

```json
{"type": "ERROR", "message": "Room is full"}
{"type": "ERROR", "message": "Not your turn"}
{"type": "ERROR", "message": "Cannot check, must call or fold"}
{"type": "ERROR", "message": "No game in progress"}
{"type": "ERROR", "message": "Cannot start game (need 2+ players)"}
```

---

## 版本歷史

- **v1.0**: 文本協議
- **v1.2**: 添加 GAMESTATE 命令
- **v2.0**: JSON 格式，完整遊戲邏輯
  - 發送玩家手牌
  - 輪次通知
  - 階段變更
  - 攤牌顯示

---

## 相關文件

- [client.py](client/client.py) - Python 客戶端
- [pokerai.py](client/pokerai.py) - AI 客戶端
- [Room.cpp](server/Room.cpp) - 遊戲邏輯
- [Session.cpp](server/Session.cpp) - 消息處理
