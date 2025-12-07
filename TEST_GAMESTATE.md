# 測試新的 GAMESTATE 命令

## 快速測試

### 1. 啟動伺服器
```bash
cd build
./TexasHoldemServer 8888
```

### 2. 啟動客戶端並測試

#### 終端1（Alice）
```bash
cd client
python client.py localhost 8888

> join Alice 1000
? Joined room as Alice

> status
?? Room 1 - Players: 1/10, Game: Waiting

> gamestate
# 應該顯示詳細遊戲狀態
```

#### 終端2（Bob）
```bash
cd client
python client.py localhost 8888

> join Bob 1000
> start

> gamestate
# 查看遊戲開始後的狀態
```

## 預期結果

### STATUS 命令
```
> status
?? Room 1 - Players: 2/10, Game: In Progress
```

### GAMESTATE 命令
```
> gamestate

==================================================
?? Pot: $30 | Current Bet: $20
?? Community Cards: (flop後會顯示公共牌)

?? Players:
  Alice ($990) [Active]
  Bob ($980) [Active]
==================================================
```

## 差異對比

| 命令 | 資訊量 | 用途 |
|------|--------|------|
| `STATUS` | 簡單 | 快速查看房間基本狀態 |
| `GAMESTATE` | 詳細 | 查看完整遊戲狀態（彩池、下注、牌面、玩家） |

## 測試檢查清單

- [ ] STATUS 命令顯示房間基本信息
- [ ] GAMESTATE 命令顯示詳細遊戲狀態
- [ ] 彩池金額正確顯示
- [ ] 當前下注正確顯示
- [ ] 公共牌正確顯示（發牌後）
- [ ] 所有玩家信息正確顯示
- [ ] 玩家狀態（Active/Folded等）正確

## 問題排查

如果 GAMESTATE 沒有顯示預期結果：

1. **檢查伺服器日誌**
   - 確認命令被接收
   - 確認狀態被正確格式化

2. **檢查客戶端輸出**
   - 確認收到 GAMESTATE 回應
   - 確認解析邏輯正確

3. **手動測試協議**
```bash
telnet localhost 8888
JOIN Alice 1000
GAMESTATE
```

應該收到類似：
```
GAMESTATE|0,0|1,Alice,1000,0
```

---

**測試通過標準**: 
? STATUS 和 GAMESTATE 都能正確顯示對應的狀態信息
