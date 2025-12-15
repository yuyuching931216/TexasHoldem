#include "Game.h"
#include "Player.h"
#include "HandEvaluator.h"
#include <iostream>
#include <stdexcept>
#include <random>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <locale>
#include <memory>

// ───────────────────────────────
// 建構子
// ───────────────────────────────
PokerGame::PokerGame()
    : currentPlayer_(0), pot_(0), currentBet_(0) { 
    // 不設置 locale，避免數字格式問題（如千位分隔符）
}

// ───────────────────────────────
// 遊戲開始
// ───────────────────────────────
void PokerGame::startGame() {
    // 重置所有玩家狀態
    resetPlayersForNewHand();
    
    // 洗牌
    deck_.shuffle();
    
    // 清空公共牌
    communityCards_.clear();
    
    // 設置盲注位置（如果有足夠玩家）
    setupBlinds();
    
    // 發底牌
    dealHoleCards();
    
    // 處理下注回合
    processBettingRound(BettingRound::PRE_FLOP);
    
    // 如果還有多於1個活躍玩家，繼續遊戲
    if (getActivePlayerCount() > 1) {
        // 發公共牌並處理下注
        dealFlop();
        processBettingRound(BettingRound::FLOP);
        
        if (getActivePlayerCount() > 1) {
            dealTurn();
            processBettingRound(BettingRound::TURN);
            
            if (getActivePlayerCount() > 1) {
                dealRiver();
                processBettingRound(BettingRound::RIVER);
            }
        }
    }
    
    // 進行攤牌和判定勝負
    showdownAndDetermineWinner();
    
    // 顯示最終遊戲狀態
    std::cout << getGameState() << std::endl;
}

// ───────────────────────────────
// 攤牌和判定勝負（正確的邊池計算）
// ───────────────────────────────
void PokerGame::showdownAndDetermineWinner() {
    std::cout << "\n=== Showdown ===" << std::endl;
    
    // 收集參與攤牌的玩家（未棄牌的玩家）
    std::vector<Player*> playersInShowdown;
    for (auto& player : players_) {
        if (player.isActive() || player.isAllIn()) {
            playersInShowdown.push_back(&player);
        }
    }
    
    if (playersInShowdown.empty()) {
        std::cout << "No players in showdown!" << std::endl;
        return;
    }
    
    // 只有一個玩家，直接獲勝（其他人都棄牌了）
    if (playersInShowdown.size() == 1) {
        Player* winner = playersInShowdown[0];
        winner->addChips(pot_);
        winner->incrementHandsWon();
        
        std::cout << winner->getName() << " wins $" << pot_ << " (no contest)" << std::endl;
        std::cout << winner->getName() << " shows: [" << winner->getHandString() << "]" << std::endl;
        
        pot_ = 0;
        
        // 更新統計
        for (auto& player : players_) {
            if (player.isActive() || player.isAllIn() || player.isFolded()) {
                player.incrementHandsPlayed();
            }
        }
        return;
    }
    
    // 評估每個玩家的手牌
    using PlayerHandPair = std::pair<Player*, HandValue>;
    std::vector<PlayerHandPair> playerHands;
    playerHands.reserve(playersInShowdown.size());
    
    for (Player* player : playersInShowdown) {
        std::vector<Card> allCards = player->getHand();
        allCards.insert(allCards.end(), communityCards_.begin(), communityCards_.end());
        HandValue handValue = HandEvaluator::evaluateHand(allCards);
        playerHands.emplace_back(player, std::move(handValue));
        
        std::cout << player->getName() << " shows: [" << player->getHandString() 
                  << "] - " << playerHands.back().second.toString() 
                  << " (Total bet: $" << player->getTotalBetThisHand() << ")" << std::endl;
    }
    
    // 檢查是否有 all-in 玩家
    bool hasAllIn = false;
    for (Player* p : playersInShowdown) {
        if (p->isAllIn()) {
            hasAllIn = true;
            break;
        }
    }
    
    if (!hasAllIn) {
        // ═══════════════════════════════════════════════════════════
        // 簡單情況：沒有 all-in，所有玩家投注相同，直接分配給最大牌
        // ═══════════════════════════════════════════════════════════
        std::sort(playerHands.begin(), playerHands.end(), 
                  [](const PlayerHandPair& a, const PlayerHandPair& b) {
                      return a.second > b.second;
                  });
        
        std::vector<Player*> winners;
        const HandValue& winningHand = playerHands[0].second;
        
        for (const auto& playerHand : playerHands) {
            if (playerHand.second == winningHand) {
                winners.push_back(playerHand.first);
            } else {
                break;
            }
        }
        
        int winAmount = pot_ / static_cast<int>(winners.size());
        int remainder = pot_ % static_cast<int>(winners.size());
        
        std::cout << "\n=== Winner" << (winners.size() > 1 ? "s" : "") << " ===" << std::endl;
        
        for (size_t i = 0; i < winners.size(); ++i) {
            int amount = winAmount;
            if (static_cast<int>(i) < remainder) {
                amount++;
            }
            
            winners[i]->addChips(amount);
            winners[i]->incrementHandsWon();
            
            if (winners.size() == 1) {
                std::cout << winners[i]->getName() << " wins $" << amount 
                          << " with " << winningHand.toString() << std::endl;
            } else {
                std::cout << winners[i]->getName() << " wins $" << amount 
                          << " (split pot) with " << winningHand.toString() << std::endl;
            }
        }
        
        pot_ = 0;
    } else {
        // ═══════════════════════════════════════════════════════════
        // 複雜情況：有 all-in，需要計算邊池
        // ═══════════════════════════════════════════════════════════
        std::cout << "\n=== Side Pot Calculation ===" << std::endl;
        
        // 收集所有玩家的總投注額
        struct PlayerBetInfo {
            Player* player;
            int totalBet;
            bool eligible;
            HandValue handValue;
        };
        
        std::vector<PlayerBetInfo> allBets;
        for (auto& player : players_) {
            bool eligible = (player.isActive() || player.isAllIn());
            HandValue hv;
            
            if (eligible) {
                for (const auto& ph : playerHands) {
                    if (ph.first == &player) {
                        hv = ph.second;
                        break;
                    }
                }
            }
            
            allBets.push_back({&player, player.getTotalBetThisHand(), eligible, hv});
        }
        
        // 收集所有不同的投注層級
        std::vector<int> betLevels;
        for (const auto& pb : allBets) {
            if (pb.totalBet > 0) {
                bool found = false;
                for (int level : betLevels) {
                    if (level == pb.totalBet) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    betLevels.push_back(pb.totalBet);
                }
            }
        }
        std::sort(betLevels.begin(), betLevels.end());
        
        // 計算每個邊池
        struct SidePot {
            int amount;
            std::vector<Player*> eligiblePlayers;
        };
        
        std::vector<SidePot> sidePots;
        int previousLevel = 0;
        
        for (int level : betLevels) {
            int potAmount = 0;
            std::vector<Player*> eligible;
            
            for (const auto& pb : allBets) {
                if (pb.totalBet >= previousLevel) {
                    int contribution = std::min(level, pb.totalBet) - previousLevel;
                    if (contribution > 0) {
                        potAmount += contribution;
                    }
                    
                    if (pb.eligible && pb.totalBet >= level) {
                        eligible.push_back(pb.player);
                    }
                }
            }
            
            if (potAmount > 0) {
                sidePots.push_back({potAmount, eligible});
                std::string potName = (sidePots.size() == 1) ? "Main Pot" : ("Side Pot " + std::to_string(sidePots.size() - 1));
                std::cout << potName << ": $" << potAmount << " (";
                for (size_t i = 0; i < eligible.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << eligible[i]->getName();
                }
                std::cout << " eligible)" << std::endl;
            }
            
            previousLevel = level;
        }
        
        // 分配每個邊池
        std::cout << "\n=== Winners ===" << std::endl;
        
        for (size_t potIndex = 0; potIndex < sidePots.size(); ++potIndex) {
            const auto& sidePot = sidePots[potIndex];
            
            if (sidePot.eligiblePlayers.empty()) {
                continue;
            }
            
            std::string potName = (potIndex == 0) ? "Main Pot" : ("Side Pot " + std::to_string(potIndex));
            
            if (sidePot.eligiblePlayers.size() == 1) {
                Player* winner = sidePot.eligiblePlayers[0];
                winner->addChips(sidePot.amount);
                if (potIndex == 0) winner->incrementHandsWon();
                
                std::cout << winner->getName() << " wins " << potName 
                          << " ($" << sidePot.amount << ") - no contest" << std::endl;
                continue;
            }
            
            // 找出這個邊池中牌最大的玩家
            std::vector<std::pair<Player*, HandValue>> eligibleHands;
            for (Player* p : sidePot.eligiblePlayers) {
                for (const auto& ph : playerHands) {
                    if (ph.first == p) {
                        eligibleHands.emplace_back(p, ph.second);
                        break;
                    }
                }
            }
            
            std::sort(eligibleHands.begin(), eligibleHands.end(),
                      [](const auto& a, const auto& b) { return a.second > b.second; });
            
            std::vector<Player*> potWinners;
            const HandValue& bestHand = eligibleHands[0].second;
            
            for (const auto& eh : eligibleHands) {
                if (eh.second == bestHand) {
                    potWinners.push_back(eh.first);
                } else {
                    break;
                }
            }
            
            int share = sidePot.amount / static_cast<int>(potWinners.size());
            int rem = sidePot.amount % static_cast<int>(potWinners.size());
            
            for (size_t i = 0; i < potWinners.size(); ++i) {
                int award = share;
                if (static_cast<int>(i) < rem) award++;
                
                potWinners[i]->addChips(award);
                if (potIndex == 0 && i == 0) potWinners[i]->incrementHandsWon();
                
                if (potWinners.size() == 1) {
                    std::cout << potWinners[i]->getName() << " wins " << potName 
                              << " ($" << award << ") with " << bestHand.toString() << std::endl;
                } else {
                    std::cout << potWinners[i]->getName() << " wins " << potName 
                              << " ($" << award << ", split) with " << bestHand.toString() << std::endl;
                }
            }
        }
        
        pot_ = 0;
    }
    
    // 更新所有玩家的遊戲統計
    for (auto& player : players_) {
        if (player.isActive() || player.isAllIn() || player.isFolded()) {
            player.incrementHandsPlayed();
        }
    }
}

// ───────────────────────────────
// 重置玩家狀態為新手牌
// ───────────────────────────────
void PokerGame::resetPlayersForNewHand() {
    for (auto& player : players_) {
        player.resetForNewHand();
        if (player.getChips() > 0 && player.isConnected()) {
            player.setState(PlayerState::ACTIVE);
        }
    }
    currentPlayer_ = 0;
    pot_ = 0;
    currentBet_ = 0;
}

// ───────────────────────────────
// 設置盲注位置
// ───────────────────────────────
void PokerGame::setupBlinds() {
    if (players_.size() < 2) return;
    
    // 重置所有玩家的位置狀態
    for (auto& player : players_) {
        player.setDealer(false);
        player.setSmallBlind(false);
        player.setBigBlind(false);
    }
    
    // 設置莊家、小盲、大盲
    if (players_.size() == 2) {
        players_[0].setDealer(true);
        players_[0].setSmallBlind(true);
        players_[1].setBigBlind(true);
    } else {
        players_[0].setDealer(true);
        players_[1].setSmallBlind(true);
        players_[2].setBigBlind(true);
    }
}

// ───────────────────────────────
// 獲取大盲注玩家的索引
// ───────────────────────────────
int PokerGame::getBigBlindIndex() const {
    for (size_t i = 0; i < players_.size(); ++i) {
        if (players_[i].isBigBlind()) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// ───────────────────────────────
// 獲取小盲注玩家的索引
// ───────────────────────────────
int PokerGame::getSmallBlindIndex() const {
    for (size_t i = 0; i < players_.size(); ++i) {
        if (players_[i].isSmallBlind()) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// ───────────────────────────────
// 獲取下一個活躍玩家的索引
// ───────────────────────────────
int PokerGame::getNextActivePlayerIndex(int startIndex) const {
    if (players_.empty()) return -1;
    
    int playerCount = static_cast<int>(players_.size());
    for (int i = 1; i <= playerCount; ++i) {
        int nextIndex = (startIndex + i) % playerCount;
        if (players_[nextIndex].isActive()) {
            return nextIndex;
        }
    }
    return -1;
}

// ───────────────────────────────
// 獲取下注順序的起始玩家索引
// ───────────────────────────────
int PokerGame::getBettingStartIndex(BettingRound round) const {
    if (round == BettingRound::PRE_FLOP) {
        // Pre-flop: 從大盲注的下一個人開始
        int bbIndex = getBigBlindIndex();
        if (bbIndex != -1) {
            return getNextActivePlayerIndex(bbIndex);
        }
    } else {
        // Post-flop: 從小盲注開始
        int sbIndex = getSmallBlindIndex();
        if (sbIndex != -1 && players_[sbIndex].isActive()) {
            return sbIndex;
        } else {
            // 如果小盲注已經棄牌，找下一個活躍玩家
            return getNextActivePlayerIndex(sbIndex);
        }
    }
    return 0;
}

// ───────────────────────────────
// 發底牌 (每位玩家兩張)
// ───────────────────────────────
void PokerGame::dealHoleCards() {
    std::cout << "Dealing hole cards..." << std::endl;
    
    // 洗牌
    deck_.shuffle();
    
    // 清空公共牌
    communityCards_.clear();
    
    for (int i = 0; i < 2; ++i) {
        for (auto& player : players_) {
            if (player.isActive() || player.getState() == PlayerState::WAITING) {
                if (deck_.isEmpty()) deck_.reset();
                player.receiveCard(deck_.dealCard());
            }
        }
    }
    
    // 伺服器端顯示每位玩家的手牌（不隱藏）
    std::cout << "\n=== Hole Cards ===" << std::endl;
    for (const auto& player : players_) {
        if (!player.getHand().empty()) {
            std::cout << player.getName() << ": [" << player.getHandString() << "]" << std::endl;
        }
    }
    std::cout << "==================" << std::endl;
}

// ───────────────────────────────
// 發公共牌 (Flop)
// ───────────────────────────────
void PokerGame::dealFlop() {
    std::cout << "\n--- Flop ---" << std::endl;
    for (int i = 0; i < 3; ++i) {
        if (deck_.isEmpty()) deck_.reset();
        communityCards_.push_back(deck_.dealCard());
    }
    
    std::cout << "Flop: ";
    for (size_t i = 0; i < 3; ++i) {
        std::cout << communityCards_[i].toString();
        if (i < 2) std::cout << ", ";
    }
    std::cout << std::endl;
}

// ───────────────────────────────
// 發 Turn (轉牌)
// ───────────────────────────────
void PokerGame::dealTurn() {
    std::cout << "\n--- Turn ---" << std::endl;
    if (deck_.isEmpty()) deck_.reset();
    communityCards_.push_back(deck_.dealCard());
    
    std::cout << "Turn: " << communityCards_.back().toString() << std::endl;
}

// ───────────────────────────────
// 發 River (河牌)
// ───────────────────────────────
void PokerGame::dealRiver() {
    std::cout << "\n--- River ---" << std::endl;
    if (deck_.isEmpty()) deck_.reset();
    communityCards_.push_back(deck_.dealCard());
    
    std::cout << "River: " << communityCards_.back().toString() << std::endl;
}

// ───────────────────────────────
// 處理下注回合
// ───────────────────────────────
void PokerGame::processBettingRound(BettingRound round) {
    std::string roundName = bettingRoundToString(round);
    std::cout << "\n=== " << roundName << " Betting Round ===" << std::endl;
    
    // 重置當前回合的下注金額（除了 Pre-flop，因為有盲注）
    if (round != BettingRound::PRE_FLOP) {
        currentBet_ = 0;
        // 重置所有玩家的當前下注
        for (auto& player : players_) {
            if (player.isActive()) {
                player.setCurrentBet(0);
            }
        }
    }
    
    // 設置盲注（僅在 Pre-flop）
    if (round == BettingRound::PRE_FLOP) {
        processBlinds();
    }
    
    // 進行完整的下注回合
    runBettingRound(round);
    
    std::cout << roundName << " round ended, Pot: $" << pot_ << std::endl;
}

// ───────────────────────────────
// 處理盲注
// ───────────────────────────────
void PokerGame::processBlinds() {
    const int smallBlindAmount = 10;
    const int bigBlindAmount = 20;
    
    // 重置當前下注和彩池
    currentBet_ = 0;
    
    for (auto& player : players_) {
        if (player.isSmallBlind() && player.canAct()) {
            if (player.call(smallBlindAmount)) {
                pot_ += smallBlindAmount;
                player.setCurrentBet(smallBlindAmount);
                std::cout << player.getName() << " posts small blind: $" << smallBlindAmount << std::endl;
            }
        } else if (player.isBigBlind() && player.canAct()) {
            if (player.call(bigBlindAmount)) {
                pot_ += bigBlindAmount;
                player.setCurrentBet(bigBlindAmount);
                currentBet_ = bigBlindAmount; // 設置當前最高下注
                std::cout << player.getName() << " posts big blind: $" << bigBlindAmount << std::endl;
            }
        }
    }
}

// ───────────────────────────────
// 執行完整的下注回合
// ───────────────────────────────
void PokerGame::runBettingRound(BettingRound round) {
    // 獲取下注順序的起始位置
    int startIndex = getBettingStartIndex(round);
    if (startIndex == -1) return;
    
    // 簡化邏輯：按順序讓每個需要行動的玩家行動
    int maxRounds = static_cast<int>(players_.size()) * 3; // 防止無限循環
    int roundCount = 0;
    
    bool someoneNeedsToAct = true;
    while (someoneNeedsToAct && roundCount < maxRounds) {
        someoneNeedsToAct = false;
        int currentIndex = startIndex;
        
        // 檢查是否還有玩家需要行動
        for (size_t i = 0; i < players_.size(); ++i) {
            if (players_[currentIndex].isActive() && needsToAct(players_[currentIndex])) {
                someoneNeedsToAct = true;
                performPlayerAction(players_[currentIndex]);
                
                // 檢查是否所有人都已棄牌或只剩一人
                if (getActivePlayerCount() <= 1) {
                    return;
                }
            }
            currentIndex = getNextActivePlayerIndex(currentIndex);
            if (currentIndex == -1) break;
        }
        
        roundCount++;
    }
}

// ───────────────────────────────
// 檢查下注回合是否完成
// ───────────────────────────────
bool PokerGame::isBettingRoundComplete() {
    int activePlayers = 0;
    int playersWithCorrectBet = 0;
    
    for (const auto& player : players_) {
        if (player.isActive() || player.isAllIn()) {
            activePlayers++;
            if (player.getCurrentBet() == currentBet_ || player.isAllIn()) {
                playersWithCorrectBet++;
            }
        }
    }
    
    // 如果只有一個或沒有活躍玩家，回合結束
    if (activePlayers <= 1) {
        return true;
    }
    
    // 如果所有活躍玩家的下注都等於當前下注，回合結束
    return playersWithCorrectBet == activePlayers;
}

// ───────────────────────────────
// 獲取遊戲狀態字串
// ───────────────────────────────
std::string PokerGame::getGameState() const {
    std::stringstream state;
    
    state << "\n=== Final Game State ===\n";
    
    // 顯示所有玩家狀態
    for (const auto& player : players_) {
        state << player.getStatusString() << "\n";
        
        // 顯示手牌（遊戲結束後可以顯示）
        if (!player.getHand().empty()) {
            state << "  Final Hand: [" << player.getHandString() << "]\n";
            
            // 顯示手牌評估結果
            if (!communityCards_.empty()) {
                std::vector<Card> allCards = player.getHand();
                allCards.insert(allCards.end(), communityCards_.begin(), communityCards_.end());
                HandValue handValue = HandEvaluator::evaluateHand(allCards);
                state << "  Hand Value: " << handValue.toString() << "\n";
            }
        }
    }
    
    // 顯示公共牌
    if (!communityCards_.empty()) {
        state << "\nFinal Community Cards:\n";
        for (const auto& card : communityCards_) {
            state << "  " << card.toString() << "\n";
        }
    }
    
    // 顯示彩池
    state << "\nFinal Pot: $" << pot_ << "\n";
    state << "Current Bet: $" << currentBet_ << "\n";
    
    // 顯示遊戲統計
    state << "\nFinal Player Statistics:\n";
    for (const auto& player : players_) {
        double winRate = player.getWinRate();
        state << "  " << player.getName() 
              << " - Chips: $" << player.getChips()
              << ", Win Rate: " << std::fixed << std::setprecision(1) << winRate << "%"
              << ", Games Played: " << player.getHandsPlayed()
              << ", Games Won: " << player.getHandsWon() << "\n";
    }
    
    return state.str();
}

// ───────────────────────────────
// 加入玩家
// ───────────────────────────────
void PokerGame::addPlayer(const Player& player) {
    // 創建玩家副本並設置初始狀態
    Player newPlayer = player;
    newPlayer.setPosition(static_cast<int>(players_.size()));
    newPlayer.setConnected(true);
    newPlayer.setState(PlayerState::WAITING);
    
    players_.push_back(newPlayer);
    
    std::cout << "Player " << newPlayer.getName() << " joined the game with $" 
              << newPlayer.getChips() << " chips" << std::endl;
}

// ───────────────────────────────
// 檢查玩家是否可以 check
// ───────────────────────────────
bool PokerGame::canPlayerCheck(const Player& player) const {
    return currentBet_ == player.getCurrentBet();
}

// ───────────────────────────────
// 獲取活躍玩家數量
// ───────────────────────────────
int PokerGame::getActivePlayerCount() const {
    int count = 0;
    for (const auto& player : players_) {
        if (player.isActive()) {
            count++;
        }
    }
    return count;
}

// ───────────────────────────────
// 獲取彩池金額
// ───────────────────────────────
int PokerGame::getPot() const {
    return pot_;
}

// ───────────────────────────────
// 獲取當前下注金額
// ───────────────────────────────
int PokerGame::getCurrentBet() const {
    return currentBet_;
}

// ───────────────────────────────
// 檢查玩家是否需要行動
// ───────────────────────────────
bool PokerGame::needsToAct(const Player& player) {
    if (!player.isActive()) {
        return false;
    }
    
    // 如果玩家的下注少於當前下注，需要行動
    return player.getCurrentBet() < currentBet_;
}

// ───────────────────────────────
// 執行玩家動作
// ───────────────────────────────
void PokerGame::performPlayerAction(Player& player) {
    // 計算需要跟注的金額
    int amountToCall = currentBet_ - player.getCurrentBet();
    
    // 簡單的隨機動作模擬
    int action = rand() % 4;
    
    switch (action) {
        case 0: // Fold
            if (player.fold()) {
                std::cout << player.getName() << " folds" << std::endl;
            }
            break;
        case 1: // Call
            if (amountToCall > 0) {
                if (player.call(currentBet_)) {
                    pot_ += amountToCall;
                    player.setCurrentBet(currentBet_);
                    std::cout << player.getName() << " calls $" << amountToCall << std::endl;
                }
            } else {
                // 如果沒有需要跟注的金額，則 check
                if (player.check()) {
                    std::cout << player.getName() << " checks" << std::endl;
                }
            }
            break;
        case 2: // Raise
            {
                int raiseAmount = 40;
                int newTotalBet = currentBet_ + raiseAmount;
                int playerNeedsToPay = newTotalBet - player.getCurrentBet();
                
                if (player.call(newTotalBet)) {
                    pot_ += playerNeedsToPay;
                    player.setCurrentBet(newTotalBet);
                    currentBet_ = newTotalBet; // 更新當前最高下注
                    std::cout << player.getName() << " raises to $" << newTotalBet << std::endl;
                } else {
                    // 如果無法加注，嘗試跟注
                    if (amountToCall > 0 && player.call(currentBet_)) {
                        pot_ += amountToCall;
                        player.setCurrentBet(currentBet_);
                        std::cout << player.getName() << " calls $" << amountToCall << std::endl;
                    }
                }
            }
            break;
        case 3: // Check or Call
            if (amountToCall == 0) {
                // 可以 check
                if (player.check()) {
                    std::cout << player.getName() << " checks" << std::endl;
                }
            } else {
                // 需要跟注
                if (player.call(currentBet_)) {
                    pot_ += amountToCall;
                    player.setCurrentBet(currentBet_);
                    std::cout << player.getName() << " calls $" << amountToCall << std::endl;
                }
            }
            break;
    }
}

// ───────────────────────────────
// 轉換下注回合為字串
// ───────────────────────────────
std::string PokerGame::bettingRoundToString(BettingRound round) {
    switch (round) {
        case BettingRound::PRE_FLOP:
            return "Pre-flop";
        case BettingRound::FLOP:
            return "Flop";
        case BettingRound::TURN:
            return "Turn";
        case BettingRound::RIVER:
            return "River";
        default:
            return "Unknown";
    }
}