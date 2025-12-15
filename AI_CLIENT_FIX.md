# AI 客戶端修復說明

## ? 原始問題

`pokerai.py` 無法與伺服器正常交互，原因：

### 1. 協議不匹配

| 項目 | 原始 AI | 伺服器 |
|------|---------|--------|
| **發送格式** | JSON: `{"action": "RAISE"}` | 文本: `ACTION RAISE 10` |
| **接收格式** | 期望 JSON | 發送文本: `STATE|...` |
| **加入命令** | ? 沒有 | 需要: `JOIN name buyin` |

### 2. 缺少關鍵功能

- ? 沒有發送 `JOIN` 命令加入遊戲
- ? 無法解析伺服器的文本協議
- ? 無法正確處理遊戲狀態更新

---

## ? 修復後

### 協議相容

| 功能 | 修復後 |
|------|--------|
| **連接** | ? 正確連接並發送 `JOIN` |
| **接收** | ? 解析文本協議 `STATE|...` |
| **發送** | ? 使用文本格式 `ACTION FOLD` |

### 新增功能

1. **自動加入遊戲**
   ```python
   self.send(f"JOIN {self.name} 1000")
   ```

2. **解析遊戲狀態**
   ```python
   def parse_game_state(self, state_data):
       # 解析: pot,currentBet,communityCards|player1|player2...
   ```

3. **解析牌面**
   ```python
   def parse_card(card_str):
       # 支援: "2 of Hearts", "Ace of Spades", "2H"
   ```

4. **自動決策**
   ```python
   def consider_action(self):
       action, amount = decide_action(...)
       self.send(f"ACTION {action} {amount}")
   ```

---

## 使用方法

### 啟動伺服器
```bash
./build/TexasHoldemServer 8888
```

### 啟動 AI 客戶端
```bash
# 默認連接
python client/pokerai.py

# 指定主機和端口
python client/pokerai.py localhost 8888

# 指定 AI 名稱
python client/pokerai.py localhost 8888 AI_Player1
```

### 啟動多個 AI
```bash
# 終端1
python client/pokerai.py localhost 8888 AI_1

# 終端2
python client/pokerai.py localhost 8888 AI_2

# 終端3（人類玩家）
python client/client.py localhost 8888
> join Human 1000
> start
```

---

## AI 決策邏輯

### 勝率估算
使用蒙特卡洛模擬估算勝率：
```python
equity = estimate_equity(hole, board, num_opponents, sims=500)
```

### 決策門檻

| 階段 | Raise 門檻 | Call 門檻 |
|------|-----------|-----------|
| Pre-flop | 65% | 45% |
| Flop | 55% | 35% |
| Turn | 50% | 30% |
| River | 50% | 25% |

### Bluff 機率

| 階段 | Bluff 機率 |
|------|-----------|
| Pre-flop | 2% |
| Flop | 8% |
| Turn | 12% |
| River | 15% |

---

## 目前限制

### ?? 已知問題

1. **手牌未知**
   - 伺服器目前不發送玩家的手牌
   - AI 使用隨機模擬手牌
   - 需要伺服器擴展來發送私有手牌

2. **輪次判斷**
   - 無法確切知道是否輪到自己
   - 需要伺服器添加 `YOUR_TURN` 消息

3. **下注金額**
   - 無法獲取自己當前的下注金額
   - 需要伺服器擴展狀態信息

### 建議的伺服器擴展

```cpp
// 發送給特定玩家的私有信息
void Room::sendPrivateState(int playerId) {
    // 發送手牌
    send("HOLE|" + player.getHandString());
    
    // 發送輪到你了
    send("YOUR_TURN|" + currentBet);
}
```

---

## 測試場景

### 場景1：AI vs AI
```bash
# 終端1: 伺服器
./TexasHoldemServer 8888

# 終端2: AI 1
python pokerai.py localhost 8888 AI_1

# 終端3: AI 2
python pokerai.py localhost 8888 AI_2

# 終端4: 觀察者（人類）
python client.py localhost 8888
> join Observer 1000
> start
> gamestate
```

### 場景2：人類 vs AI
```bash
# 終端1: 伺服器
./TexasHoldemServer 8888

# 終端2: AI
python pokerai.py localhost 8888 AI_Bot

# 終端3: 人類玩家
python client.py localhost 8888
> join Human 1000
> start
> gamestate
> call
> raise 50
```

---

## 相關文件

- [client.py](client/client.py) - 人類玩家客戶端
- [Session.cpp](server/Session.cpp) - 伺服器命令處理
- [Room.cpp](server/Room.cpp) - 遊戲房間邏輯
- [PROTOCOL_UPDATE.md](PROTOCOL_UPDATE.md) - 協議說明

---

**修復狀態**: ? **完成**  
**測試狀態**: ?? **需要伺服器擴展以完整支持**

---

## 下一步改進

### 優先級高
1. 伺服器發送 `HOLE|card1,card2` 給玩家
2. 伺服器發送 `YOUR_TURN|playerId` 通知輪到誰

### 優先級中
3. 添加下注歷史記錄
4. 添加玩家籌碼信息

### 優先級低
5. 改進 AI 策略（位置感知、對手建模）
6. 添加機器學習模型
