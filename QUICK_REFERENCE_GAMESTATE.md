# 快速參考：狀態查詢命令

## 兩種狀態查詢方式

### 1. STATUS - 簡單快速
```
> status
?? Room 1 - Players: 3/10, Game: In Progress
```

**返回信息**：
- 房間 ID
- 玩家數量
- 遊戲狀態

**適用場景**：
- ? 快速檢查房間狀態
- ? 頻繁輪詢
- ? 網路流量敏感

---

### 2. GAMESTATE - 詳細完整
```
> gamestate

==================================================
?? Pot: $150 | Current Bet: $50
?? Community Cards: 2?, 5?, K?

?? Players:
  Alice ($950) [Active]
  Bob ($850) [Active]
  Charlie ($1100) [Folded]
==================================================
```

**返回信息**：
- 彩池金額
- 當前下注
- 公共牌
- 所有玩家狀態（籌碼、狀態）

**適用場景**：
- ? 查看完整遊戲狀態
- ? 做決策前確認局面
- ? 斷線重連後同步
- ? 調試和測試

---

## 使用示例

### 場景1：剛加入房間
```bash
> join Alice 1000
? Joined room as Alice

> status              # 快速查看
?? Room 1 - Players: 1/10, Game: Waiting
```

### 場景2：遊戲進行中
```bash
> gamestate           # 查看詳細狀態
# 顯示彩池、下注、公共牌、所有玩家信息

> call                # 根據狀態做決策
```

### 場景3：斷線重連
```bash
> gamestate           # 立即同步完整狀態
# 了解當前局面，繼續遊戲
```

---

## 協議格式

### STATUS 回應
```
STATUS|Room <id> - Players: <current>/<max>, Game: <status>
```

### GAMESTATE 回應
```
GAMESTATE|<pot>,<bet>[,<cards>]|<player1>|<player2>|...
```

---

## 命令對比

| 特性 | STATUS | GAMESTATE |
|------|--------|-----------|
| **信息量** | 簡單 | 詳細 |
| **網路流量** | 小 | 中 |
| **響應速度** | 快 | 快 |
| **房間信息** | ? | ? |
| **彩池/下注** | ? | ? |
| **公共牌** | ? | ? |
| **玩家詳情** | ? | ? |
| **適合輪詢** | ? | ?? |
| **適合調試** | ?? | ? |

---

## 實現細節

### 伺服器端
```cpp
// Room.h
std::string getRoomState() const;   // STATUS 使用
std::string getGameState() const;   // GAMESTATE 使用

// Session.cpp
if (command == "STATUS") {
    send("STATUS|" + room_->getRoomState());
}
else if (command == "GAMESTATE") {
    send("GAMESTATE|" + room_->getGameState());
}
```

### 客戶端
```python
# client.py
> status      # 發送 "STATUS" 命令
> gamestate   # 發送 "GAMESTATE" 命令
```

---

## 版本歷史

- **v1.0**: 只有 STATUS 命令（簡單狀態）
- **v1.2**: 添加 GAMESTATE 命令（詳細狀態）?

---

## 相關文檔

- [PROTOCOL_UPDATE.md](PROTOCOL_UPDATE.md) - 完整協議說明
- [TEST_GAMESTATE.md](TEST_GAMESTATE.md) - 測試指南
- [FIX_GAMESTATE_SUMMARY.md](FIX_GAMESTATE_SUMMARY.md) - 修復總結

---

**提示**: 根據需求選擇適當的命令，避免不必要的網路流量！
