# 死鎖問題修復說明

## 問題描述

原始代碼中存在資源死鎖（resource deadlock）問題，錯誤信息：
```
Error processing message: resource deadlock would occur
```

## 死鎖原因

### 原始問題代碼

在 `Room.cpp` 中：

```cpp
bool Room::canStartGame() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.size() >= MIN_PLAYERS && !gameInProgress_;
}

bool Room::addPlayer(...) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (isFull()) {  // ❌ 問題：isFull() 會再次嘗試獲取已被鎖定的 mutex_
        return false;
    }
    // ...
}

bool Room::isFull() const {
    std::lock_guard<std::mutex> lock(mutex_);  // ❌ 死鎖：mutex_ 已經被 addPlayer 鎖定
    return sessions_.size() >= MAX_PLAYERS;
}
```

### 死鎖場景

1. `addPlayer()` 獲取 `mutex_` 鎖
2. `addPlayer()` 調用 `isFull()`
3. `isFull()` 嘗試獲取 `mutex_` 鎖
4. **死鎖**：`mutex_` 已被同一線程鎖定，無法再次獲取

這是一個**遞歸鎖問題**（recursive lock），在使用 `std::mutex` 時不支持遞歸鎖定。

## 修復方案

### 方案 1: 直接訪問（已採用）

在已持有鎖的方法中，直接訪問成員變量，不調用其他需要鎖的方法：

```cpp
bool Room::addPlayer(...) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // ✅ 修復：直接檢查，不調用 isFull()
    if (sessions_.size() >= MAX_PLAYERS) {
        return false;
    }
    // ...
}
```

### 方案 2: 鎖分離策略

將需要在鎖外執行的操作分離出來：

```cpp
bool Room::addPlayer(...) {
    int playerId;
    std::string message;
    
    {
        // 只在必要時持有鎖
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (sessions_.size() >= MAX_PLAYERS) {
            return false;
        }
        
        // 在鎖內準備數據
        playerId = nextPlayerId_++;
        // ...
        message = prepareMessage();
    } // 鎖在這裡釋放
    
    // ✅ 在鎖外執行可能阻塞的操作
    broadcast(message);
}
```

### 方案 3: Unsafe 輔助方法

為需要在持有鎖時調用的方法提供不加鎖的版本：

```cpp
// 公共接口 - 加鎖
std::string Room::getPlayerList() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return getPlayerListUnsafe();
}

// 私有方法 - 不加鎖（調用者必須持有鎖）
std::string Room::getPlayerListUnsafe() const {
    // 直接訪問 sessions_，不加鎖
    std::stringstream ss;
    // ...
    return ss.str();
}

// 使用
bool Room::addPlayer(...) {
    std::lock_guard<std::mutex> lock(mutex_);
    // ...
    
    // ✅ 調用 unsafe 版本，因為我們已經持有鎖
    std::string list = getPlayerListUnsafe();
}
```

## 修復後的代碼結構

### Room.h

```cpp
class Room {
public:
    // 公共接口 - 自己管理鎖
    bool isFull() const;
    bool isEmpty() const;
    bool canStartGame() const;
    
private:
    mutable std::mutex mutex_;
    
    // Unsafe 版本 - 調用者必須持有鎖
    std::string getPlayerListUnsafe() const;
    std::string formatGameStateUnsafe() const;
};
```

### Room.cpp 關鍵修改

```cpp
bool Room::addPlayer(...) {
    int playerId;
    std::string joinMessage;
    std::string playerListMessage;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // ✅ 直接檢查，不調用 isFull()
        if (sessions_.size() >= MAX_PLAYERS) {
            return false;
        }
        
        // 準備數據
        playerId = nextPlayerId_++;
        // ...
        
        // ✅ 調用 unsafe 版本
        playerListMessage = "PLAYERS|" + getPlayerListUnsafe();
    } // 鎖釋放
    
    // ✅ 在鎖外發送消息
    broadcast(joinMessage);
    sendToPlayer(playerId, playerListMessage);
    
    return true;
}

void Room::broadcast(...) {
    std::map<int, std::shared_ptr<Session>> sessionsCopy;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // ✅ 複製會話列表
        sessionsCopy = sessions_;
    } // 鎖釋放
    
    // ✅ 在鎖外遍歷和發送
    for (const auto& [playerId, session] : sessionsCopy) {
        session->send(message + "\n");
    }
}
```

## 鎖策略最佳實踐

### 1. 最小鎖範圍

```cpp
// ❌ 不好：持有鎖太久
void badFunction() {
    std::lock_guard<std::mutex> lock(mutex_);
    doA();
    doB();
    expensiveOperation();  // 耗時操作
    doC();
}

// ✅ 好：只在必要時持有鎖
void goodFunction() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        doA();
        doB();
    }
    
    expensiveOperation();  // 在鎖外執行
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        doC();
    }
}
```

### 2. 避免嵌套鎖調用

```cpp
// ❌ 不好：方法互相調用，都加鎖
void functionA() {
    std::lock_guard<std::mutex> lock(mutex_);
    functionB();  // 死鎖！
}

void functionB() {
    std::lock_guard<std::mutex> lock(mutex_);
    // ...
}

// ✅ 好：使用內部無鎖版本
void functionA() {
    std::lock_guard<std::mutex> lock(mutex_);
    functionBUnsafe();  // 安全
}

void functionB() {
    std::lock_guard<std::mutex> lock(mutex_);
    functionBUnsafe();
}

void functionBUnsafe() {
    // 不加鎖，由調用者保證
}
```

### 3. 鎖外執行 I/O

```cpp
// ❌ 不好：持有鎖時執行 I/O
void badFunction() {
    std::lock_guard<std::mutex> lock(mutex_);
    prepareData();
    session->send(data);  // I/O 操作，可能阻塞
}

// ✅ 好：先複製數據，鎖外執行 I/O
void goodFunction() {
    std::string data;
    std::shared_ptr<Session> sessionCopy;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        data = prepareData();
        sessionCopy = session;
    }
    
    sessionCopy->send(data);  // 在鎖外執行
}
```

### 4. 使用 std::recursive_mutex（如果必要）

如果確實需要遞歸鎖定：

```cpp
class Room {
private:
    mutable std::recursive_mutex mutex_;  // 支持遞歸鎖定
    
public:
    bool isFull() const {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        return sessions_.size() >= MAX_PLAYERS;
    }
    
    bool addPlayer(...) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (isFull()) {  // ✅ 可以工作，但不推薦
            return false;
        }
        // ...
    }
};
```

**注意**：雖然 `std::recursive_mutex` 可以解決問題，但通常表示設計可以改進。

## 檢查清單

在多線程代碼中，檢查以下事項以避免死鎖：

- [ ] **避免嵌套鎖調用**：同一線程不要重複獲取同一個非遞歸鎖
- [ ] **最小化鎖範圍**：只在必要時持有鎖
- [ ] **鎖外執行 I/O**：網路、文件操作在鎖外執行
- [ ] **統一鎖順序**：如果需要多個鎖，始終以相同順序獲取
- [ ] **使用 RAII**：使用 `lock_guard` 或 `unique_lock`，避免忘記解鎖
- [ ] **避免在持有鎖時調用外部代碼**：特別是回調函數
- [ ] **考慮無鎖數據結構**：如果可能，使用無鎖替代方案

## 測試方法

### 檢測死鎖

1. **編譯時檢查**：啟用 `-fsanitize=thread` (GCC/Clang)
2. **運行時檢查**：使用 ThreadSanitizer
3. **壓力測試**：大量並發請求
4. **代碼審查**：檢查所有加鎖的地方

### 測試命令

```bash
# 編譯時啟用線程檢查
g++ -std=c++17 -g -fsanitize=thread -o server *.cpp

# 運行測試
./server 8888

# 在另一個終端，啟動多個客戶端
for i in {1..10}; do
    python client.py localhost 8888 &
done
```

## 總結

本次修復通過以下方式解決了死鎖問題：

1. ✅ 移除嵌套鎖調用
2. ✅ 提供 unsafe 內部方法
3. ✅ 縮小鎖的範圍
4. ✅ 在鎖外執行 I/O 操作
5. ✅ 複製數據以避免長時間持有鎖

這些改進不僅修復了死鎖，還提高了性能和可維護性。

## 相關資源

- [C++ Mutex Reference](https://en.cppreference.com/w/cpp/thread/mutex)
- [Lock-Free Programming](https://preshing.com/20120612/an-introduction-to-lock-free-programming/)
- [ThreadSanitizer](https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual)
