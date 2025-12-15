# 快速測試指南 - JSON 協議

## 編譯

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

或在 Visual Studio 中直接編譯。

---

## 測試場景

### 場景 1: 人類 vs 人類

**終端 1 - 啟動伺服器**
```bash
./TexasHoldemServer 8888
```

**終端 2 - 玩家 Alice**
```bash
cd client
python client.py localhost 8888

> join Alice 1000
> status
```

**終端 3 - 玩家 Bob**
```bash
cd client
python client.py localhost 8888

> join Bob 1000
> start
```

**預期結果：**
- 兩位玩家收到 `HOLE_CARDS` (手牌)
- 第一位玩家收到 `YOUR_TURN`
- 遊戲狀態顯示 pot, current_bet, community_cards

---

### 場景 2: 人類 vs AI

**終端 1 - 伺服器**
```bash
./TexasHoldemServer 8888
```

**終端 2 - AI**
```bash
cd client
python pokerai.py localhost 8888 AI_Bot
```

**終端 3 - 人類玩家**
```bash
cd client
python client.py localhost 8888

> join Human 1000
> start
```

**預期結果：**
- AI 自動加入
- AI 收到手牌後自動決策
- 人類玩家看到 AI 的動作

---

### 場景 3: 多個 AI

**終端 1 - 伺服器**
```bash
./TexasHoldemServer 8888
```

**終端 2-5 - AI 玩家**
```bash
python pokerai.py localhost 8888 AI_1
python pokerai.py localhost 8888 AI_2
python pokerai.py localhost 8888 AI_3
python pokerai.py localhost 8888 AI_4
```

**終端 6 - 觀察者/啟動者**
```bash
python client.py localhost 8888

> join Observer 1000
> start
> gamestate
```

**預期結果：**
- 所有 AI 自動行動
- 遊戲自動進行到結束

---

## 測試命令

### 查看房間狀態
```
> status
```
輸出：
```json
{
    "type": "ROOM_STATE",
    "room_id": 1,
    "player_count": 2,
    "max_players": 10,
    "game_in_progress": true,
    "stage": "flop"
}
```

### 查看遊戲狀態
```
> gamestate
```
輸出：完整的遊戲狀態，包括公共牌、玩家信息

### 玩家動作
```
> fold       # 棄牌
> check      # 過牌
> call       # 跟注
> raise 100  # 加注到 100
> allin      # 全下
```

---

## 驗證清單

### 基本功能
- [ ] 伺服器啟動正常
- [ ] 客戶端可以連接
- [ ] JOIN 命令有效
- [ ] 收到 JSON 格式的回應

### 遊戲流程
- [ ] 遊戲可以開始
- [ ] 玩家收到手牌 (HOLE_CARDS)
- [ ] 輪到玩家時收到 YOUR_TURN
- [ ] 動作被正確處理
- [ ] 階段正確變更 (preflop → flop → turn → river)
- [ ] 遊戲正確結束

### AI 功能
- [ ] AI 可以連接並加入
- [ ] AI 收到手牌
- [ ] AI 自動做出決策
- [ ] AI 決策合理（基於勝率）

### JSON 格式
- [ ] 所有消息都是有效的 JSON
- [ ] 牌面格式正確 (rank, suit, short)
- [ ] 玩家狀態正確 (0-4)

---

## 常見問題

### Q: AI 不行動？

**檢查：**
1. AI 是否收到 `HOLE_CARDS`？
2. AI 是否收到 `YOUR_TURN`？
3. 檢查 AI 輸出日誌

### Q: 遊戲卡住？

**檢查：**
1. 查看伺服器日誌
2. 使用 `gamestate` 查看當前狀態
3. 確認當前玩家是否活躍

### Q: JSON 解析錯誤？

**檢查：**
1. 確認消息以 `\n` 結尾
2. 檢查 JSON 格式是否正確
3. 使用 `json.loads()` 測試

---

## 使用 telnet 測試

```bash
telnet localhost 8888

JOIN Alice 1000
STATUS
GAMESTATE
QUIT
```

---

## 日誌

### 伺服器日誌
伺服器會輸出：
- 玩家加入/離開
- 收到的消息
- 遊戲狀態變更

### 客戶端日誌
客戶端顯示：
- 收到的 JSON 消息
- 遊戲狀態
- 手牌信息

### AI 日誌
AI 輸出：
- `[AI] Equity: 0.65` - 勝率
- `[AI] Sent: ACTION CALL` - 發送的動作
- `[AI] ?? My hand: [('A', 'h'), ('K', 's')]` - 手牌

---

## 成功標準

? 伺服器穩定運行  
? JSON 消息正確格式化  
? 遊戲流程完整  
? AI 自動行動  
? 無死鎖或錯誤  
