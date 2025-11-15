#pragma once
#include <vector>
#include <string>
#include "Card.h"

// 前置宣告
class Player;

class PokerGame {
    public:
        PokerGame();
        
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
        void processBettingRound(const std::string& roundName);
        void processBlinds();
        void runBettingRound();
        bool isBettingRoundComplete();
        bool needsToAct(const Player& player);
        void performPlayerAction(Player& player);
        bool canPlayerCheck(const Player& player) const;
        
        // 遊戲狀態管理
        void resetPlayersForNewHand();
        void setupBlinds();
        
        // 獲取遊戲資訊
        int getActivePlayerCount() const;
        int getPot() const;
        int getCurrentBet() const;
        const std::vector<Player>& getPlayers() const { return players_; }
        const std::vector<Card>& getCommunityCards() const { return communityCards_; }
        
    private:
        Deck deck_;
        std::vector<Card> communityCards_;
        std::vector<Player> players_;
        int currentPlayer_;
        int pot_;
        int currentBet_;  // 當前回合的最高下注金額
    };