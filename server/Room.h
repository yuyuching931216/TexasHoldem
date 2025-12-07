#pragma once
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <map>
#include "Player.h"
#include "Card.h"
#include "Game.h"

class Session;

// 遊戲房間類別 - 管理一局德州撲克遊戲
class Room : public std::enable_shared_from_this<Room> {
public:
    static constexpr size_t MAX_PLAYERS = 10;
    static constexpr size_t MIN_PLAYERS = 2;
    
    Room(int roomId);
    ~Room() = default;
    
    // 房間管理
    int getRoomId() const { return roomId_; }
    bool isFull() const;
    bool isEmpty() const;
    size_t getPlayerCount() const;
    bool canStartGame() const;
    
    // 玩家管理
    bool addPlayer(std::shared_ptr<Session> session, const std::string& playerName, int buyIn = 1000);
    void removePlayer(int playerId);
    std::shared_ptr<Player> getPlayer(int playerId);
    
    // 遊戲控制
    void startGame();
    void processPlayerAction(int playerId, const std::string& action, int amount = 0);
    
    // 廣播消息給所有玩家
    void broadcast(const std::string& message, int excludePlayerId = -1);
    
    // 獲取房間狀態
    std::string getRoomState() const;      // 簡單狀態（房間信息）
    std::string getGameState() const;      // 詳細狀態（遊戲信息）
    std::string getPlayerList() const;
    
private:
    int roomId_;
    std::map<int, std::shared_ptr<Session>> sessions_;  // playerId -> session
    std::unique_ptr<PokerGame> game_;
    mutable std::mutex mutex_;
    int nextPlayerId_;
    bool gameInProgress_;
    
    // 輔助函數
    void sendToPlayer(int playerId, const std::string& message);
    void notifyGameState();
    std::string formatGameState() const;
    
    // 無鎖版本（必須在持有鎖時調用）
    std::string formatGameStateUnsafe() const;
    std::string getPlayerListUnsafe() const;
};
