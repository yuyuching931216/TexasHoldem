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

// 遊戲階段枚舉
enum class GameStage {
    WAITING,
    PRE_FLOP,
    FLOP,
    TURN,
    RIVER,
    SHOWDOWN
};

// 遊戲房間類別 - 管理一局德州撲克遊戲
class Room : public std::enable_shared_from_this<Room> {
public:
    static constexpr size_t MAX_PLAYERS = 10;
    static constexpr size_t MIN_PLAYERS = 2;
    static constexpr int SMALL_BLIND = 15;   // 小盲 15
    static constexpr int BIG_BLIND = 30;     // 大盲 30
    static constexpr int AUTO_START_DELAY_MS = 3000;  // 自動開始延遲（毫秒）
    
    Room(int roomId, bool autoStart = true);
    ~Room() = default;
    
    // 房間管理
    int getRoomId() const { return roomId_; }
    bool isFull() const;
    bool isEmpty() const;
    size_t getPlayerCount() const;
    bool canStartGame() const;
    
    // 自動開始設定
    void setAutoStart(bool enabled) { autoStart_ = enabled; }
    bool isAutoStartEnabled() const { return autoStart_; }
    
    // 玩家管理
    bool addPlayer(std::shared_ptr<Session> session, const std::string& playerName, int buyIn = 1000);
    void removePlayer(int playerId);
    std::shared_ptr<Player> getPlayer(int playerId);
    
    // 遊戲控制
    void startGame();
    void tryAutoStartGame();  // 嘗試自動開始遊戲
    void processPlayerAction(int playerId, const std::string& action, int amount = 0);
    
    // 廣播消息給所有玩家
    void broadcast(const std::string& message, int excludePlayerId = -1);
    
    // 獲取房間狀態（JSON 格式）
    std::string getRoomStateJson() const;
    std::string getGameStateJson() const;
    std::string getPlayerListJson() const;
    
    // 舊格式（向後兼容）
    std::string getRoomState() const;
    std::string getGameState() const;
    std::string getPlayerList() const;
    
private:
    int roomId_;
    std::map<int, std::shared_ptr<Session>> sessions_;  // playerId -> session
    std::unique_ptr<PokerGame> game_;
    mutable std::mutex mutex_;
    int nextPlayerId_;
    bool gameInProgress_;
    bool autoStart_;              // 是否啟用自動開始
    bool autoStartPending_;       // 是否正在等待自動開始
    GameStage currentStage_;
    int currentPlayerIndex_;      // 當前行動的玩家索引
    int dealerIndex_;             // 莊家索引
    int lastRaisePlayerId_;       // 最後加注的玩家
    
    // 輔助函數
    void sendToPlayer(int playerId, const std::string& message);
    void notifyGameState();
    void notifyPlayerTurn(int playerId);
    void sendHoleCards();
    void dealCommunityCards();
    void advanceToNextPlayer();
    void advanceToNextStage();
    void processShowdown();
    bool isRoundComplete() const;
    int getNextActivePlayer(int fromIndex) const;
    std::string stageToString(GameStage stage) const;
    void printCommunityCards() const;  // 伺服器端顯示公共牌
    
    // JSON 格式化
    std::string formatGameStateJson() const;
    std::string formatPlayerJson(const Player& player, bool includeHand = false) const;
    std::string formatCardJson(const Card& card) const;
    std::string formatCardsJson(const std::vector<Card>& cards) const;
    
    // 無鎖版本（必須在持有鎖時調用）
    std::string formatGameStateUnsafe() const;
    std::string getPlayerListUnsafe() const;
    std::string formatGameStateJsonUnsafe() const;
};
