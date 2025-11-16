#pragma once
#include <vector>
#include <string>
#include <memory>
#include "Card.h"

// 前置宣告
class Player;

// 下注回合枚舉
enum class BettingRound {
    PRE_FLOP,
    FLOP,
    TURN,
    RIVER
};

class PokerGame {
    public:
        PokerGame();
        ~PokerGame() = default;
        
        // 遊戲控制
        void startGame();
        void addPlayer(const Player& player);
        std::string getGameState() const;
        
        // 發牌相關
        void dealHoleCards();
        void dealFlop();
        void dealTurn();
        void dealRiver();
        
        // 下注相關
        void processBettingRound(BettingRound round);
        void processBlinds();
        void runBettingRound(BettingRound round);
        bool isBettingRoundComplete();
        bool needsToAct(const Player& player);
        void performPlayerAction(Player& player);
        bool canPlayerCheck(const Player& player) const;
        
        // 攤牌和勝負判定
        void showdownAndDetermineWinner();
        
        // 玩家順序管理
        int getBigBlindIndex() const;
        int getSmallBlindIndex() const;
        int getNextActivePlayerIndex(int startIndex) const;
        int getBettingStartIndex(BettingRound round) const;
        
        // 遊戲狀態管理
        void resetPlayersForNewHand();
        void setupBlinds();
        
        // 獲取遊戲資訊
        int getActivePlayerCount() const;
        int getPot() const;
        int getCurrentBet() const;
        const std::vector<Player>& getPlayers() const { return players_; }
        const std::vector<Card>& getCommunityCards() const { return communityCards_; }
        
        // 工具方法
        static std::string bettingRoundToString(BettingRound round);
        
    private:
        Deck deck_;
        std::vector<Card> communityCards_;
        std::vector<Player> players_;
        int currentPlayer_;
        int pot_;
        int currentBet_;  // 當前回合的最高下注金額
    };