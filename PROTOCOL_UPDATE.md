# 通訊協議更新說明

## v1.2 - 狀態查詢改進

### 問題

之前的實現中：
- `STATUS` 命令只返回簡單的房間信息（玩家數量、遊戲狀態）
- `formatGameState()` 方法生成詳細的遊戲狀態，但客戶端無法訪問
- 客戶端無法查詢當前的遊戲詳情（彩池、下注、公共牌等）

### 解決方案

新增 `GAMESTATE` 命令，專門用於查詢詳細的遊戲狀態。

---

## 更新的通訊協議

### 客戶端 → 伺服器

| 命令 | 格式 | 說明 | 回應 |
|------|------|------|------|
| `JOIN` | `JOIN <name> [buyin]` | 加入房間 | `OK` 或 `ERROR` |
| `START` | `START` | 開始遊戲 | `GAME_START` 或 `ERROR` |
| `ACTION` | `ACTION <action> [amount]` | 執行動作 | `ACTION` 廣播 |
| `STATUS` | `STATUS` | 查詢房間狀態 | `STATUS` |
| `GAMESTATE` | `GAMESTATE` | 查詢詳細遊戲狀態 | `GAMESTATE` ? |
| `QUIT` | `QUIT` | 離開房間 | `BYE` |

### 狀態格式

**STATUS 回應**：簡單房間狀態
```
STATUS|Room 1 - Players: 3/10, Game: In Progress
```

**GAMESTATE 回應**：詳細遊戲狀態
```
GAMESTATE|<pot>,<currentBet>[,<cards>]|<player1>|<player2>|...
```

範例：
```
GAMESTATE|150,50,2H;5D;KC|1,Alice,950,0|2,Bob,850,0
```

---

## 使用範例

### 查詢房間狀態
```bash
> status
?? Room 1 - Players: 3/10, Game: In Progress
```

### 查詢詳細遊戲狀態
```bash
> gamestate

==================================================
?? Pot: $150 | Current Bet: $50
?? Community Cards: 2?, 5?, K?

?? Players:
  Alice ($950) [Active]
  Bob ($850) [Active]
==================================================
```

---

## 實現

### Room.h
```cpp
std::string getRoomState() const;   // 簡單狀態
std::string getGameState() const;   // 詳細狀態 ?
```

### Session.cpp
```cpp
else if (command == "GAMESTATE") {  // ? 新增
    send("GAMESTATE|" + room_->getGameState() + "\n");
}
```

### client.py
```python
elif command == "gamestate":  # ? 新增
    self.send_message("GAMESTATE")
```

---

**更新日期**: 2024  
**版本**: v1.2  
**狀態**: ? 已實現
