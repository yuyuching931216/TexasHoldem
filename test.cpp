#include "server/Card.h"
#include "server/Player.h"
#include "server/Game.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>  // 添加這個來使用 setprecision
#include <ctime>    // 添加這個來使用 time
#include <cstdlib>  // 添加這個來使用 rand 和 srand
#include <locale>   // 添加這個來處理編碼

class TexasHoldemTester {
private:
    PokerGame game_;
    std::vector<Player> players_;
    int currentRound_;
    
public:
    TexasHoldemTester() : currentRound_(0) {
        // 設置控制台輸出編碼
        #ifdef _WIN32
        std::locale::global(std::locale(""));
        std::cout.imbue(std::locale(""));
        #endif
    }
    
    // 初始化遊戲
    void initializeGame() {
        std::cout << "=== Texas Hold'em Local Test ===" << std::endl;
        std::cout << "Initializing game..." << std::endl;
        
        // 清空現有玩家
        players_.clear();
        
        // 創建玩家
        players_.push_back(Player(1, "Alice", 1000));
        players_.push_back(Player(2, "Bob", 1000));
        players_.push_back(Player(3, "Charlie", 1000));
        players_.push_back(Player(4, "Diana", 1000));
        
        // 設置玩家位置
        for (size_t i = 0; i < players_.size(); ++i) {
            players_[i].setPosition(static_cast<int>(i));
            game_.addPlayer(players_[i]);
        }
        
        // 設置盲注位置
        if (players_.size() >= 2) {
            players_[0].setDealer(true);
            players_[1].setSmallBlind(true);
            players_[2 % players_.size()].setBigBlind(true);
        }
        
        std::cout << "Game initialization completed!" << std::endl;
        displayPlayers();
    }
    
    // 顯示玩家資訊
    void displayPlayers() {
        std::cout << "\n--- Player Information ---" << std::endl;
        for (const auto& player : players_) {
            std::cout << player.getStatusString() << std::endl;
        }
        std::cout << std::endl;
    }
    
    // 測試發牌功能
    void testDealCards() {
        std::cout << "=== Testing Deal Cards ===" << std::endl;
        
        // 重置玩家手牌
        for (auto& player : players_) {
            player.clearHand();
            player.resetForNewHand();
        }
        
        // 測試發底牌
        Deck testDeck;
        std::cout << "Dealing hole cards to each player..." << std::endl;
        
        for (int cardNum = 0; cardNum < 2; ++cardNum) {
            for (auto& player : players_) {
                if (!testDeck.isEmpty()) {
                    Card card = testDeck.dealCard();
                    player.receiveCard(card);
                }
            }
        }
        
        // 顯示玩家手牌
        for (const auto& player : players_) {
            std::cout << player.getName() << "'s hand: [" 
                      << player.getHandString() << "]" << std::endl;
        }
        
        // 測試公共牌
        std::vector<Card> communityCards;
        std::cout << "\nDealing community cards..." << std::endl;
        
        // Flop (3張)
        std::cout << "Flop: ";
        for (int i = 0; i < 3; ++i) {
            if (!testDeck.isEmpty()) {
                Card card = testDeck.dealCard();
                communityCards.push_back(card);
                std::cout << card.toString();
                if (i < 2) std::cout << ", ";
            }
        }
        std::cout << std::endl;
        
        // Turn (1張)
        if (!testDeck.isEmpty()) {
            Card turn = testDeck.dealCard();
            communityCards.push_back(turn);
            std::cout << "Turn: " << turn.toString() << std::endl;
        }
        
        // River (1張)
        if (!testDeck.isEmpty()) {
            Card river = testDeck.dealCard();
            communityCards.push_back(river);
            std::cout << "River: " << river.toString() << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    // 測試下注功能
    void testBettingActions() {
        std::cout << "=== Testing Betting Actions ===" << std::endl;
        
        // 設置小盲和大盲
        int smallBlind = 10;
        int bigBlind = 20;
        
        std::cout << "Setting blinds - Small Blind: $" << smallBlind 
                  << ", Big Blind: $" << bigBlind << std::endl;
        
        // 小盲下注
        for (auto& player : players_) {
            if (player.isSmallBlind()) {
                player.call(smallBlind);
                std::cout << player.getName() << " posts small blind: $" << smallBlind << std::endl;
            }
            if (player.isBigBlind()) {
                player.call(bigBlind);
                std::cout << player.getName() << " posts big blind: $" << bigBlind << std::endl;
            }
        }
        
        // 模擬玩家動作
        std::cout << "\nSimulating betting round..." << std::endl;
        
        for (auto& player : players_) {
            if (!player.isSmallBlind() && !player.isBigBlind()) {
                // 模擬不同的動作
                int action = rand() % 4;
                
                switch (action) {
                    case 0: // Fold
                        if (player.fold()) {
                            std::cout << player.getName() << " folds" << std::endl;
                        }
                        break;
                    case 1: // Call
                        if (player.call(bigBlind)) {
                            std::cout << player.getName() << " calls $" << bigBlind << std::endl;
                        }
                        break;
                    case 2: // Raise
                        if (player.raise(40)) {
                            std::cout << player.getName() << " raises to $" << (bigBlind + 40) << std::endl;
                        }
                        break;
                    case 3: // Check (如果可以)
                        if (player.check()) {
                            std::cout << player.getName() << " checks" << std::endl;
                        } else if (player.call(bigBlind)) {
                            std::cout << player.getName() << " calls $" << bigBlind << std::endl;
                        }
                        break;
                }
            }
        }
        
        std::cout << "\nPlayer status after betting round:" << std::endl;
        displayPlayers();
    }
    
    // 測試完整遊戲流程
    void testCompleteGame() {
        std::cout << "=== Testing Complete Game Flow ===" << std::endl;
        
        currentRound_++;
        std::cout << "Starting round " << currentRound_ << std::endl;
        
        // 重置玩家狀態
        for (auto& player : players_) {
            player.resetForNewHand();
            if (player.getChips() > 0) {
                player.setState(PlayerState::ACTIVE);
            }
        }
        
        // 使用內建的遊戲功能
        try {
            game_.startGame();
            std::cout << "\nGame state:\n" << game_.getGameState() << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Error occurred during game execution: " << e.what() << std::endl;
        }
    }
    
    // 測試牌組功能
    void testDeckFunctionality() {
        std::cout << "=== Testing Deck Functionality ===" << std::endl;
        
        Deck testDeck;
        std::cout << "Creating new deck..." << std::endl;
        
        // 顯示前10張牌
        std::cout << "First 10 cards:" << std::endl;
        for (int i = 0; i < 10 && !testDeck.isEmpty(); ++i) {
            Card card = testDeck.dealCard();
            std::cout << (i + 1) << ". " << card.toString() << std::endl;
        }
        
        std::cout << "\nReshuffling..." << std::endl;
        testDeck.reset();
        
        std::cout << "First 5 cards after reshuffle:" << std::endl;
        for (int i = 0; i < 5 && !testDeck.isEmpty(); ++i) {
            Card card = testDeck.dealCard();
            std::cout << (i + 1) << ". " << card.toString() << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    // 顯示遊戲統計
    void displayGameStats() {
        std::cout << "=== Game Statistics ===" << std::endl;
        std::cout << "Rounds played: " << currentRound_ << std::endl;
        std::cout << "Number of players: " << players_.size() << std::endl;
        
        int activePlayers = 0;
        int totalChips = 0;
        
        for (const auto& player : players_) {
            if (player.isActive()) activePlayers++;
            totalChips += player.getChips();
            
            std::cout << player.getName() << ": "
                      << "Chips=$" << player.getChips() 
                      << ", Status=" << (player.isActive() ? "Active" : 
                                       player.isFolded() ? "Folded" : "Other")
                      << ", Win Rate=" << std::fixed << std::setprecision(1) 
                      << player.getWinRate() << "%" << std::endl;
        }
        
        std::cout << "Active players: " << activePlayers << std::endl;
        std::cout << "Total chips: $" << totalChips << std::endl;
        std::cout << std::endl;
    }
    
    // 主要測試流程
    void runAllTests() {
        std::cout << "Starting Texas Hold'em tests..." << std::endl << std::endl;
        
        initializeGame();
        testDeckFunctionality();
        testDealCards();
        testBettingActions();
        testCompleteGame();
        displayGameStats();
        
        std::cout << "All tests completed!" << std::endl;
    }
    
    // 互動式遊戲模式
    void interactiveMode() {
        std::cout << "=== Interactive Game Mode ===" << std::endl;
        std::cout << "Enter command (help for instructions):" << std::endl;
        
        std::string command;
        while (true) {
            std::cout << "> ";
            std::getline(std::cin, command);
            
            if (command == "help") {
                std::cout << "Available commands:" << std::endl;
                std::cout << "  init - Initialize game" << std::endl;
                std::cout << "  deal - Deal cards" << std::endl;
                std::cout << "  bet - Test betting" << std::endl;
                std::cout << "  game - Start game" << std::endl;
                std::cout << "  stats - Show statistics" << std::endl;
                std::cout << "  players - Show players" << std::endl;
                std::cout << "  exit - Exit" << std::endl;
            }
            else if (command == "init") {
                initializeGame();
            }
            else if (command == "deal") {
                testDealCards();
            }
            else if (command == "bet") {
                testBettingActions();
            }
            else if (command == "game") {
                testCompleteGame();
            }
            else if (command == "stats") {
                displayGameStats();
            }
            else if (command == "players") {
                displayPlayers();
            }
            else if (command == "exit") {
                std::cout << "Exiting game..." << std::endl;
                break;
            }
            else {
                std::cout << "Unknown command, type 'help' for instructions" << std::endl;
            }
        }
    }
};

// 測試程式的 main 函數
int main() {
    // 設置隨機種子
    srand(static_cast<unsigned int>(time(nullptr)));
    
    TexasHoldemTester tester;
    
    std::cout << "Texas Hold'em Test Program" << std::endl;
    std::cout << "1. Run all tests" << std::endl;
    std::cout << "2. Interactive mode" << std::endl;
    std::cout << "Please choose (1 or 2): ";
    
    int choice;
    std::cin >> choice;
    std::cin.ignore(); // 忽略換行符
    
    if (choice == 1) {
        tester.runAllTests();
    } else if (choice == 2) {
        tester.interactiveMode();
    } else {
        std::cout << "Invalid choice, running default tests..." << std::endl;
        tester.runAllTests();
    }
    
    return 0;
}