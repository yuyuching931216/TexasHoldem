#include "Room.h"
#include "Session.h"
#include "HandEvaluator.h"
#include "Logger.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>

Room::Room(int roomId, bool autoStart)
    : roomId_(roomId), game_(std::make_unique<PokerGame>()), 
      nextPlayerId_(1), gameInProgress_(false), autoStart_(autoStart),
      autoStartPending_(false), currentStage_(GameStage::WAITING),
      currentPlayerIndex_(0), dealerIndex_(0), lastRaisePlayerId_(-1) {
    LOG_INFO("Room", "Room " + std::to_string(roomId_) + " created (auto-start: " + 
             (autoStart_ ? "enabled" : "disabled") + ")");
}

bool Room::isFull() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.size() >= MAX_PLAYERS;
}

bool Room::isEmpty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.empty();
}

size_t Room::getPlayerCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.size();
}

bool Room::canStartGame() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.size() >= MIN_PLAYERS && !gameInProgress_;
}

bool Room::addPlayer(std::shared_ptr<Session> session, const std::string& playerName, int buyIn) {
    int playerId;
    std::string joinMessage;
    std::string playerListMessage;
    bool shouldTryAutoStart = false;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (sessions_.size() >= MAX_PLAYERS) {
            return false;
        }
        
        playerId = nextPlayerId_++;
        session->setPlayerId(playerId);
        sessions_[playerId] = session;
        
        // 創建玩家並加入遊戲
        Player player(playerId, playerName, buyIn);
        player.setConnectionId(playerId);
        player.setConnected(true);
        game_->addPlayer(player);
        
        // JSON 格式的加入消息
        std::stringstream ss;
        ss << "{\"type\":\"JOINED\",\"player_id\":" << playerId 
           << ",\"name\":\"" << playerName << "\",\"chips\":" << buyIn << "}";
        joinMessage = ss.str();
        
        playerListMessage = "{\"type\":\"PLAYERS\",\"players\":" + getPlayerListUnsafe() + "}";
        
        LOG_INFO("Room", "Player " + playerName + " (ID: " + std::to_string(playerId) + 
                 ") joined room " + std::to_string(roomId_) + " (" + 
                 std::to_string(sessions_.size()) + "/" + std::to_string(MAX_PLAYERS) + " players)");
        
        // 檢查是否應該嘗試自動開始
        if (autoStart_ && !gameInProgress_ && sessions_.size() >= MIN_PLAYERS && !autoStartPending_) {
            shouldTryAutoStart = true;
        }
    }
    
    // 在鎖外發送消息
    broadcast(joinMessage);
    sendToPlayer(playerId, playerListMessage);
    
    // 嘗試自動開始遊戲
    if (shouldTryAutoStart) {
        tryAutoStartGame();
    }
    
    return true;
}

void Room::tryAutoStartGame() {
    bool canStart = false;
    size_t playerCount = 0;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        canStart = (autoStart_ && !gameInProgress_ && sessions_.size() >= MIN_PLAYERS && !autoStartPending_);
        playerCount = sessions_.size();
        
        if (canStart) {
            autoStartPending_ = true;
        }
    }
    
    if (!canStart) {
        return;
    }
    
    // 通知玩家遊戲即將開始
    std::stringstream ss;
    ss << "{\"type\":\"AUTO_START_COUNTDOWN\",\"seconds\":" << (AUTO_START_DELAY_MS / 1000) 
       << ",\"player_count\":" << playerCount
       << ",\"message\":\"Game will start in " << (AUTO_START_DELAY_MS / 1000) << " seconds...\"}";
    broadcast(ss.str());
    
    LOG_INFO("Room", "Auto-start countdown: " + std::to_string(AUTO_START_DELAY_MS / 1000) + 
             " seconds (" + std::to_string(playerCount) + " players)");
    
    // 在新線程中延遲啟動（避免阻塞）
    std::thread([this, playerCount]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(AUTO_START_DELAY_MS));
        
        bool shouldStart = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (autoStart_ && !gameInProgress_ && sessions_.size() >= MIN_PLAYERS) {
                shouldStart = true;
            }
            autoStartPending_ = false;
        }
        
        if (shouldStart) {
            LOG_INFO("Room", "Auto-starting game with " + std::to_string(playerCount) + " players...");
            startGame();
        } else {
            broadcast("{\"type\":\"AUTO_START_CANCELLED\",\"message\":\"Auto-start cancelled (not enough players)\"}");
            LOG_INFO("Room", "Auto-start cancelled");
        }
    }).detach();
}

void Room::removePlayer(int playerId) {
    std::string message;
    bool cancelAutoStart = false;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = sessions_.find(playerId);
        if (it != sessions_.end()) {
            sessions_.erase(it);
            
            std::stringstream ss;
            ss << "{\"type\":\"LEFT\",\"player_id\":" << playerId << "}";
            message = ss.str();
            
            LOG_INFO("Room", "Player " + std::to_string(playerId) + " left room " + 
                     std::to_string(roomId_) + " (" + std::to_string(sessions_.size()) + 
                     "/" + std::to_string(MAX_PLAYERS) + " players)");
            
            if (autoStartPending_ && sessions_.size() < MIN_PLAYERS) {
                cancelAutoStart = true;
            }
        }
    }
    
    if (!message.empty()) {
        // 在鎖外發送
        broadcast(message);
    }
    
    if (cancelAutoStart) {
        broadcast("{\"type\":\"AUTO_START_CANCELLED\",\"message\":\"Auto-start cancelled (player left)\"}");
    }
}

std::shared_ptr<Player> Room::getPlayer(int playerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const auto& players = game_->getPlayers();
    for (const auto& player : players) {
        if (player.getId() == playerId) {
            return std::make_shared<Player>(player);
        }
    }
    return nullptr;
}

void Room::startGame() {
    bool canStart;
    int firstPlayerId = -1;
    int pot = 0;
    std::vector<std::pair<int, int>> rebuyPlayers;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        canStart = (sessions_.size() >= MIN_PLAYERS && !gameInProgress_);
        
        if (canStart) {
            gameInProgress_ = true;
            autoStartPending_ = false;
            currentStage_ = GameStage::PRE_FLOP;
            
            game_->resetPlayersForNewHand();
            
            // 檢查並補充籌碼為 0 的玩家
            auto& players = const_cast<std::vector<Player>&>(game_->getPlayers());
            for (auto& player : players) {
                if (player.getChips() <= 0 && player.isConnected()) {
                    // 補充籌碼
                    const int REBUY_AMOUNT = 1000;
                    player.setChips(REBUY_AMOUNT);
                    player.setState(PlayerState::ACTIVE);
                    rebuyPlayers.push_back({player.getId(), REBUY_AMOUNT});
                    LOG_INFO("Room", player.getName() + " receives rebuy: $" + std::to_string(REBUY_AMOUNT));
                }
            }
            
            game_->dealHoleCards();
            game_->setupBlinds();
            
            // 處理盲注 - 從玩家籌碼中扣除並加入 pot
            for (auto& player : players) {
                if (player.isSmallBlind() && player.getChips() >= SMALL_BLIND) {
                    player.removeChips(SMALL_BLIND);
                    player.setCurrentBet(SMALL_BLIND);
                    pot += SMALL_BLIND;
                    LOG_INFO("Room", player.getName() + " posts small blind: $" + std::to_string(SMALL_BLIND));
                }
                if (player.isBigBlind() && player.getChips() >= BIG_BLIND) {
                    player.removeChips(BIG_BLIND);
                    player.setCurrentBet(BIG_BLIND);
                    pot += BIG_BLIND;
                    LOG_INFO("Room", player.getName() + " posts big blind: $" + std::to_string(BIG_BLIND));
                }
            }
            
            game_->setPot(pot);
            game_->setCurrentBet(BIG_BLIND);
            
            // 設置莊家和起始玩家
            dealerIndex_ = 0;
            currentPlayerIndex_ = (dealerIndex_ + 3) % players.size(); // UTG 位置
            if (players.size() == 2) {
                // 兩人遊戲：小盲先行動
                currentPlayerIndex_ = 0;
            }
            lastRaisePlayerId_ = -1;
            
            // 記錄第一個玩家的 ID
            if (currentPlayerIndex_ >= 0 && currentPlayerIndex_ < static_cast<int>(players.size())) {
                firstPlayerId = players[currentPlayerIndex_].getId();
            }
            
            // 在伺服器端顯示所有玩家的手牌
            LOG_INFO("Room", "=== Hole Cards Dealt ===");
            for (const auto& player : players) {
                LOG_INFO("Room", player.getName() + ": [" + player.getHandString() + 
                         "] (Chips: $" + std::to_string(player.getChips()) + ")");
            }
            LOG_INFO("Room", "Pot: $" + std::to_string(pot) + ", Current Bet: $" + std::to_string(BIG_BLIND));
        }
    }
    
    if (!canStart) {
        broadcast("{\"type\":\"ERROR\",\"message\":\"Not enough players to start game\"}");
        return;
    }
    
    // 發送補充籌碼通知
    for (const auto& [playerId, amount] : rebuyPlayers) {
        std::stringstream ss;
        ss << "{\"type\":\"REBUY\",\"player_id\":" << playerId 
           << ",\"amount\":" << amount 
           << ",\"message\":\"Player received rebuy chips\"}";
        broadcast(ss.str());
    }
    
    // 發送遊戲開始通知
    std::stringstream ss;
    ss << "{\"type\":\"GAME_START\",\"message\":\"Game is starting...\",\"stage\":\"preflop\""
       << ",\"small_blind\":" << SMALL_BLIND << ",\"big_blind\":" << BIG_BLIND << "}";
    broadcast(ss.str());
    
    // 發送每位玩家的手牌（在鎖外處理）
    sendHoleCards();
    
    // 通知遊戲狀態
    notifyGameState();
    
    // 通知第一個玩家行動（在鎖外）
    if (firstPlayerId != -1) {
        LOG_INFO("Room", "Notifying player " + std::to_string(firstPlayerId) + " for their turn");
        notifyPlayerTurn(firstPlayerId);
    }
}

void Room::sendHoleCards() {
    std::map<int, std::string> holeCardsMessages;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        const auto& players = game_->getPlayers();
        for (const auto& player : players) {
            const auto& hand = player.getHand();
            if (hand.size() >= 2) {
                std::stringstream ss;
                ss << "{\"type\":\"HOLE_CARDS\",\"cards\":[";
                for (size_t i = 0; i < hand.size(); ++i) {
                    if (i > 0) ss << ",";
                    ss << formatCardJson(hand[i]);
                }
                ss << "]}";
                holeCardsMessages[player.getId()] = ss.str();
            }
        }
    }
    
    // 在鎖外發送
    for (const auto& [playerId, message] : holeCardsMessages) {
        sendToPlayer(playerId, message);
    }
}

void Room::notifyPlayerTurn(int playerId) {
    std::string yourTurnMessage;
    std::string currentPlayerMessage;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // 計算需要跟注的金額
        int toCall = 0;
        int playerChips = 0;
        const auto& players = game_->getPlayers();
        for (const auto& player : players) {
            if (player.getId() == playerId) {
                toCall = game_->getCurrentBet() - player.getCurrentBet();
                if (toCall < 0) toCall = 0;
                playerChips = player.getChips();
                break;
            }
        }
        
        // 計算可用動作
        std::string availableActions = "[";
        bool firstAction = true;
        
        // FOLD 總是可用
        availableActions += "\"FOLD\"";
        firstAction = false;
        
        if (toCall == 0) {
            // 沒有需要跟注的，可以 CHECK
            availableActions += ",\"CHECK\"";
        }
        
        if (toCall > 0 && playerChips > 0) {
            if (playerChips >= toCall) {
                // 有足夠籌碼跟注
                availableActions += ",\"CALL\"";
            }
            // 可以 ALL-IN（不論籌碼多少）
            availableActions += ",\"ALL_IN\"";
        }
        
        if (playerChips > toCall && playerChips >= game_->getCurrentBet() + BIG_BLIND) {
            // 有足夠籌碼加注
            availableActions += ",\"RAISE\"";
        } else if (playerChips > 0 && playerChips <= toCall) {
            // 籌碼不足以跟注，只能 ALL-IN
            availableActions += ",\"ALL_IN\"";
        }
        
        availableActions += "]";
        
        int minRaise = game_->getCurrentBet() + BIG_BLIND;
        if (minRaise > playerChips + game_->getCurrentBet()) {
            // 如果最低加注超過玩家籌碼，則 minRaise = 玩家可投入的最大值
            minRaise = playerChips + toCall;  // 這實際上就是 ALL-IN
        }
        
        std::stringstream ss;
        ss << "{\"type\":\"YOUR_TURN\",\"player_id\":" << playerId 
           << ",\"to_call\":" << toCall
           << ",\"current_bet\":" << game_->getCurrentBet()
           << ",\"pot\":" << game_->getPot()
           << ",\"your_chips\":" << playerChips
           << ",\"min_raise\":" << minRaise
           << ",\"available_actions\":" << availableActions
           << "}";
        yourTurnMessage = ss.str();
        
        currentPlayerMessage = "{\"type\":\"CURRENT_PLAYER\",\"player_id\":" + std::to_string(playerId) + "}";
    }
    
    // 在鎖外發送
    broadcast(currentPlayerMessage);
    sendToPlayer(playerId, yourTurnMessage);
    
    LOG_INFO("Room", "Sent YOUR_TURN to player " + std::to_string(playerId));
}

void Room::processPlayerAction(int playerId, const std::string& action, int amount) {
    std::string actionMessage;
    bool validAction = false;
    bool roundComplete = false;
    int nextPlayerId = -1;
    std::string errorMessage;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!gameInProgress_) {
            errorMessage = "{\"type\":\"ERROR\",\"message\":\"No game in progress\"}";
        } else {
            // 獲取當前玩家
            auto& players = const_cast<std::vector<Player>&>(game_->getPlayers());
            Player* currentPlayer = nullptr;
            int playerIndex = -1;
            
            for (size_t i = 0; i < players.size(); ++i) {
                if (players[i].getId() == playerId) {
                    currentPlayer = &players[i];
                    playerIndex = static_cast<int>(i);
                    break;
                }
            }
            
            if (!currentPlayer || playerIndex != currentPlayerIndex_) {
                errorMessage = "{\"type\":\"ERROR\",\"message\":\"Not your turn\"}";
            } else {
                // 處理動作
                int toCall = game_->getCurrentBet() - currentPlayer->getCurrentBet();
                if (toCall < 0) toCall = 0;
                int playerChips = currentPlayer->getChips();
                
                if (action == "FOLD") {
                    currentPlayer->fold();
                    currentPlayer->setHasActedThisRound(true);
                    validAction = true;
                    actionMessage = "{\"type\":\"ACTION\",\"player_id\":" + std::to_string(playerId) + 
                                   ",\"action\":\"FOLD\"}";
                    LOG_INFO("Room", currentPlayer->getName() + " FOLDS");
                }
                else if (action == "CHECK") {
                    if (toCall <= 0) {
                        currentPlayer->check();
                        currentPlayer->setHasActedThisRound(true);
                        validAction = true;
                        actionMessage = "{\"type\":\"ACTION\",\"player_id\":" + std::to_string(playerId) + 
                                       ",\"action\":\"CHECK\"}";
                        LOG_INFO("Room", currentPlayer->getName() + " CHECKS");
                    } else {
                        errorMessage = "{\"type\":\"ERROR\",\"message\":\"Cannot check, must call $" + std::to_string(toCall) + " or fold\"}";
                    }
                }
                else if (action == "CALL") {
                    if (toCall > 0) {
                        // 如果籌碼不足以跟注，視為 all-in
                        int actualCall = std::min(toCall, playerChips);
                        if (actualCall > 0 && currentPlayer->removeChips(actualCall)) {
                            currentPlayer->setCurrentBet(currentPlayer->getCurrentBet() + actualCall);
                            game_->addToPot(actualCall);
                            currentPlayer->setHasActedThisRound(true);
                            
                            // 檢查是否是 all-in
                            if (currentPlayer->getChips() == 0) {
                                currentPlayer->setState(PlayerState::ALL_IN);
                                actionMessage = "{\"type\":\"ACTION\",\"player_id\":" + std::to_string(playerId) + 
                                               ",\"action\":\"ALL_IN\",\"amount\":" + std::to_string(actualCall) + "}";
                                LOG_INFO("Room", currentPlayer->getName() + " CALLS ALL-IN for $" + std::to_string(actualCall));
                            } else {
                                actionMessage = "{\"type\":\"ACTION\",\"player_id\":" + std::to_string(playerId) + 
                                               ",\"action\":\"CALL\",\"amount\":" + std::to_string(actualCall) + "}";
                                LOG_INFO("Room", currentPlayer->getName() + " CALLS $" + std::to_string(actualCall));
                            }
                            validAction = true;
                        } else if (playerChips == 0) {
                            // 籌碼為 0，視為 check (如果可以的話)
                            if (toCall == 0) {
                                currentPlayer->check();
                                currentPlayer->setHasActedThisRound(true);
                                validAction = true;
                                actionMessage = "{\"type\":\"ACTION\",\"player_id\":" + std::to_string(playerId) + 
                                               ",\"action\":\"CHECK\"}";
                                LOG_INFO("Room", currentPlayer->getName() + " CHECKS (no chips)");
                            } else {
                                // 必須棄牌
                                currentPlayer->fold();
                                currentPlayer->setHasActedThisRound(true);
                                validAction = true;
                                actionMessage = "{\"type\":\"ACTION\",\"player_id\":" + std::to_string(playerId) + 
                                               ",\"action\":\"FOLD\"}";
                                LOG_INFO("Room", currentPlayer->getName() + " FOLDS (no chips to call)");
                            }
                        }
                    } else {
                        // 沒有需要跟注的，當作 check
                        currentPlayer->check();
                        currentPlayer->setHasActedThisRound(true);
                        validAction = true;
                        actionMessage = "{\"type\":\"ACTION\",\"player_id\":" + std::to_string(playerId) + 
                                       ",\"action\":\"CHECK\"}";
                        LOG_INFO("Room", currentPlayer->getName() + " CHECKS");
                    }
                }
                else if (action == "RAISE") {
                    int raiseTotal = amount;
                    
                    // 檢查籌碼是否足夠
                    int needToPay = raiseTotal - currentPlayer->getCurrentBet();
                    
                    if (playerChips == 0) {
                        // 沒有籌碼，無法加注
                        errorMessage = "{\"type\":\"ERROR\",\"message\":\"No chips to raise\"}";
                    } else if (needToPay >= playerChips) {
                        // 加注金額超過或等於全部籌碼，視為 all-in
                        int allInAmount = playerChips;
                        currentPlayer->removeChips(allInAmount);
                        int newBet = currentPlayer->getCurrentBet() + allInAmount;
                        currentPlayer->setCurrentBet(newBet);
                        game_->addToPot(allInAmount);
                        if (newBet > game_->getCurrentBet()) {
                            game_->setCurrentBet(newBet);
                            lastRaisePlayerId_ = playerId;
                            // When someone raises, other players who already acted need to act again
                            resetActedFlagsExcept(playerId);
                        }
                        currentPlayer->setState(PlayerState::ALL_IN);
                        currentPlayer->setHasActedThisRound(true);
                        validAction = true;
                        actionMessage = "{\"type\":\"ACTION\",\"player_id\":" + std::to_string(playerId) + 
                                       ",\"action\":\"ALL_IN\",\"amount\":" + std::to_string(allInAmount) + "}";
                        LOG_INFO("Room", currentPlayer->getName() + " goes ALL-IN for $" + std::to_string(allInAmount) + 
                                 " (wanted to raise to $" + std::to_string(raiseTotal) + ")");
                    } else if (raiseTotal > game_->getCurrentBet() && needToPay > 0) {
                        // 正常加注
                        if (currentPlayer->removeChips(needToPay)) {
                            currentPlayer->setCurrentBet(raiseTotal);
                            game_->addToPot(needToPay);
                            game_->setCurrentBet(raiseTotal);
                            lastRaisePlayerId_ = playerId;
                            // When someone raises, other players who already acted need to act again
                            resetActedFlagsExcept(playerId);
                            currentPlayer->setHasActedThisRound(true);
                            validAction = true;
                            actionMessage = "{\"type\":\"ACTION\",\"player_id\":" + std::to_string(playerId) + 
                                           ",\"action\":\"RAISE\",\"amount\":" + std::to_string(raiseTotal) + "}";
                            LOG_INFO("Room", currentPlayer->getName() + " RAISES to $" + std::to_string(raiseTotal));
                        }
                    } else {
                        errorMessage = "{\"type\":\"ERROR\",\"message\":\"Invalid raise amount\"}";
                    }
                }
                else if (action == "ALL_IN") {
                    int allInAmount = playerChips;
                    if (allInAmount > 0) {
                        currentPlayer->removeChips(allInAmount);
                        int newBet = currentPlayer->getCurrentBet() + allInAmount;
                        currentPlayer->setCurrentBet(newBet);
                        game_->addToPot(allInAmount);
                        if (newBet > game_->getCurrentBet()) {
                            game_->setCurrentBet(newBet);
                            lastRaisePlayerId_ = playerId;
                            // When someone raises, other players who already acted need to act again
                            resetActedFlagsExcept(playerId);
                        }
                        currentPlayer->setState(PlayerState::ALL_IN);
                        currentPlayer->setHasActedThisRound(true);
                        validAction = true;
                        actionMessage = "{\"type\":\"ACTION\",\"player_id\":" + std::to_string(playerId) + 
                                       ",\"action\":\"ALL_IN\",\"amount\":" + std::to_string(allInAmount) + "}";
                        LOG_INFO("Room", currentPlayer->getName() + " goes ALL-IN for $" + std::to_string(allInAmount));
                    } else {
                        // 籌碼為 0，無法 all-in，視為 check 或 fold
                        if (toCall == 0) {
                            currentPlayer->check();
                            currentPlayer->setHasActedThisRound(true);
                            validAction = true;
                            actionMessage = "{\"type\":\"ACTION\",\"player_id\":" + std::to_string(playerId) + 
                                           ",\"action\":\"CHECK\"}";
                            LOG_INFO("Room", currentPlayer->getName() + " CHECKS (no chips for all-in)");
                        } else {
                            errorMessage = "{\"type\":\"ERROR\",\"message\":\"No chips to go all-in, must fold\"}";
                        }
                    }
                }
                
                if (validAction) {
                    // 檢查回合是否完成
                    roundComplete = isRoundComplete();
                    
                    if (!roundComplete) {
                        // 找下一個玩家
                        int nextIndex = getNextActivePlayer(currentPlayerIndex_);
                        if (nextIndex != -1) {
                            currentPlayerIndex_ = nextIndex;
                            nextPlayerId = players[nextIndex].getId();
                        }
                    }
                    
                    LOG_INFO("Room", "Pot: $" + std::to_string(game_->getPot()) + ", Current Bet: $" + std::to_string(game_->getCurrentBet()));
                }
            }
        }
    }
    
    // 在鎖外發送消息
    if (!errorMessage.empty()) {
        sendToPlayer(playerId, errorMessage);
        return;
    }
    
    if (validAction) {
        broadcast(actionMessage);
        notifyGameState();
        
        if (roundComplete) {
            advanceToNextStage();
        } else if (nextPlayerId != -1) {
            notifyPlayerTurn(nextPlayerId);
        }
    }
}

bool Room::isRoundComplete() const {
    const auto& players = game_->getPlayers();
    int activePlayers = 0;
    int playersActedAndMatched = 0;
    
    for (const auto& player : players) {
        if (player.isActive()) {
            activePlayers++;
            // Player must have acted AND matched the current bet
            if (player.hasActedThisRound() && player.getCurrentBet() == game_->getCurrentBet()) {
                playersActedAndMatched++;
            }
        } else if (player.isAllIn()) {
            // All-in players count as having acted
            activePlayers++;
            playersActedAndMatched++;
        }
    }
    
    // Only one or fewer active players remaining
    if (activePlayers <= 1) {
        return true;
    }
    
    // All active players have acted and matched the current bet
    return playersActedAndMatched == activePlayers && activePlayers > 0;
}

int Room::getNextActivePlayer(int fromIndex) const {
    const auto& players = game_->getPlayers();
    int playerCount = static_cast<int>(players.size());
    
    for (int i = 1; i <= playerCount; ++i) {
        int nextIndex = (fromIndex + i) % playerCount;
        if (players[nextIndex].isActive()) {
            return nextIndex;
        }
    }
    return -1;
}

void Room::advanceToNextStage() {
    GameStage nextStage;
    int nextPlayerId = -1;
    bool shouldShowdown = false;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // 檢查活躍玩家數
        int activePlayers = 0;
        for (const auto& player : game_->getPlayers()) {
            if (player.isActive() || player.isAllIn()) {
                activePlayers++;
            }
        }
        
        // 只剩一個玩家，直接結束
        if (activePlayers <= 1) {
            shouldShowdown = true;
            currentStage_ = GameStage::SHOWDOWN;
        } else {
            // 進入下一階段
            switch (currentStage_) {
                case GameStage::PRE_FLOP:
                    currentStage_ = GameStage::FLOP;
                    game_->dealFlop();
                    LOG_INFO("Room", "=== FLOP ===");
                    printCommunityCards();
                    break;
                case GameStage::FLOP:
                    currentStage_ = GameStage::TURN;
                    game_->dealTurn();
                    LOG_INFO("Room", "=== TURN ===");
                    printCommunityCards();
                    break;
                case GameStage::TURN:
                    currentStage_ = GameStage::RIVER;
                    game_->dealRiver();
                    LOG_INFO("Room", "=== RIVER ===");
                    printCommunityCards();
                    break;
                case GameStage::RIVER:
                    shouldShowdown = true;
                    currentStage_ = GameStage::SHOWDOWN;
                    break;
                default:
                    break;
            }
            
            if (!shouldShowdown) {
                // 重置下注（新回合）
                auto& players = const_cast<std::vector<Player>&>(game_->getPlayers());
                for (auto& player : players) {
                    player.setCurrentBet(0);
                    // Reset hasActedThisRound for new betting round
                    player.setHasActedThisRound(false);
                }
                game_->setCurrentBet(0);
                
                // 從莊家後第一個活躍玩家開始
                currentPlayerIndex_ = getNextActivePlayer(dealerIndex_);
                
                if (currentPlayerIndex_ >= 0 && currentPlayerIndex_ < static_cast<int>(players.size())) {
                    nextPlayerId = players[currentPlayerIndex_].getId();
                }
            }
        }
        
        nextStage = currentStage_;
    }
    
    // 在鎖外處理
    if (shouldShowdown) {
        processShowdown();
        return;
    }
    
    // 發送階段變更通知
    std::stringstream ss;
    ss << "{\"type\":\"STAGE_CHANGE\",\"stage\":\"" << stageToString(nextStage) << "\"}";
    broadcast(ss.str());
    
    // 發送新的遊戲狀態
    notifyGameState();
    
    // 通知下一個玩家
    if (nextPlayerId != -1) {
        notifyPlayerTurn(nextPlayerId);
    }
}

void Room::printCommunityCards() const {
    const auto& cards = game_->getCommunityCards();
    std::stringstream ss;
    ss << "Community Cards: ";
    for (size_t i = 0; i < cards.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << cards[i].toString();
    }
    LOG_INFO("Room", ss.str());
    LOG_INFO("Room", "Pot: $" + std::to_string(game_->getPot()));
}

void Room::resetActedFlagsExcept(int excludePlayerId) {
    // Reset hasActedThisRound for all active players except the one who just raised
    // This is called when someone raises, so other players need to act again
    auto& players = const_cast<std::vector<Player>&>(game_->getPlayers());
    for (auto& player : players) {
        if (player.getId() != excludePlayerId && player.isActive()) {
            player.setHasActedThisRound(false);
        }
    }
}

void Room::processShowdown() {
    std::string showdownMessage;
    std::string winnerMessage;
    bool shouldStartNewGame = false;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        LOG_INFO("Room", "=== SHOWDOWN ===");
        
        auto& players = const_cast<std::vector<Player>&>(game_->getPlayers());
        std::vector<Player*> activePlayers;
        
        // 收集未棄牌的玩家
        for (auto& player : players) {
            if (player.isActive() || player.isAllIn()) {
                activePlayers.push_back(&player);
                LOG_INFO("Room", player.getName() + ": [" + player.getHandString() + 
                         "] (Total bet: $" + std::to_string(player.getTotalBetThisHand()) + ")");
            }
        }
        
        if (activePlayers.empty()) {
            LOG_INFO("Room", "No active players!");
        } else if (activePlayers.size() == 1) {
            // 只有一個玩家，直接獲勝（其他人都棄牌了）
            Player* winner = activePlayers[0];
            int winAmount = game_->getPot();
            winner->addChips(winAmount);
            winner->incrementHandsWon();
            LOG_INFO("Room", winner->getName() + " wins $" + std::to_string(winAmount) + " (others folded)");
            
            winnerMessage = "{\"type\":\"WINNER\",\"player_id\":" + std::to_string(winner->getId()) +
                           ",\"name\":\"" + winner->getName() + "\",\"amount\":" + std::to_string(winAmount) +
                           ",\"reason\":\"others folded\"}";
        } else {
            // 多個玩家攤牌 - 計算邊池
            
            // 評估每個玩家的手牌
            std::vector<std::pair<Player*, HandValue>> playerHands;
            const auto& communityCards = game_->getCommunityCards();
            
            for (Player* player : activePlayers) {
                std::vector<Card> allCards = player->getHand();
                allCards.insert(allCards.end(), communityCards.begin(), communityCards.end());
                HandValue handValue = HandEvaluator::evaluateHand(allCards);
                playerHands.emplace_back(player, handValue);
                
                LOG_INFO("Room", player->getName() + " has: " + handValue.toString());
            }
            
            // 檢查是否有 all-in 玩家
            bool hasAllIn = false;
            for (Player* p : activePlayers) {
                if (p->isAllIn()) {
                    hasAllIn = true;
                    break;
                }
            }
            
            if (!hasAllIn) {
                // ═══════════════════════════════════════════════════════════
                // 簡單情況：沒有 all-in，所有玩家投注相同，直接分配
                // ═══════════════════════════════════════════════════════════
                std::sort(playerHands.begin(), playerHands.end(),
                    [](const auto& a, const auto& b) { return a.second > b.second; });
                
                std::vector<Player*> winners;
                const HandValue& bestHand = playerHands[0].second;
                for (const auto& ph : playerHands) {
                    if (ph.second == bestHand) {
                        winners.push_back(ph.first);
                    }
                }
                
                int potShare = game_->getPot() / static_cast<int>(winners.size());
                int remainder = game_->getPot() % static_cast<int>(winners.size());
                
                std::stringstream winnerSS;
                winnerSS << "{\"type\":\"WINNER\",\"winners\":[";
                for (size_t i = 0; i < winners.size(); i++) {
                    int award = potShare;
                    if (static_cast<int>(i) < remainder) award++;
                    
                    winners[i]->addChips(award);
                    winners[i]->incrementHandsWon();
                    LOG_INFO("Room", winners[i]->getName() + " wins $" + std::to_string(award) 
                              + " with " + bestHand.toString());
                    
                    if (i > 0) winnerSS << ",";
                    winnerSS << "{\"player_id\":" << winners[i]->getId()
                             << ",\"name\":\"" << winners[i]->getName() << "\""
                             << ",\"amount\":" << award << "}";
                }
                winnerSS << "],\"hand\":\"" << bestHand.toString() << "\"}";
                winnerMessage = winnerSS.str();
            } else {
                // ═══════════════════════════════════════════════════════════
                // 複雜情況：有 all-in，需要計算邊池
                // ═══════════════════════════════════════════════════════════
                LOG_INFO("Room", "=== Side Pot Calculation ===");
                
                // 收集所有玩家的總投注額
                struct PlayerBetInfo {
                    Player* player;
                    int totalBet;
                    bool eligible;  // 是否有資格贏得彩池
                    HandValue handValue;
                };
                
                std::vector<PlayerBetInfo> allBets;
                for (auto& player : players) {
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
                            // 這個玩家貢獻了 min(level, totalBet) - previousLevel 到這個邊池
                            int contribution = std::min(level, pb.totalBet) - previousLevel;
                            if (contribution > 0) {
                                potAmount += contribution;
                            }
                            
                            // 只有投注達到這個層級且未棄牌的玩家才有資格
                            if (pb.eligible && pb.totalBet >= level) {
                                eligible.push_back(pb.player);
                            }
                        }
                    }
                    
                    if (potAmount > 0) {
                        sidePots.push_back({potAmount, eligible});
                        std::string potName = (sidePots.size() == 1) ? "Main Pot" : ("Side Pot " + std::to_string(sidePots.size() - 1));
                        std::stringstream potSS;
                        potSS << potName << ": $" << potAmount << " (";
                        for (size_t i = 0; i < eligible.size(); ++i) {
                            if (i > 0) potSS << ", ";
                            potSS << eligible[i]->getName();
                        }
                        potSS << " eligible)";
                        LOG_INFO("Room", potSS.str());
                    }
                    
                    previousLevel = level;
                }
                
                // 分配每個邊池
                LOG_INFO("Room", "=== Winners ===");
                std::stringstream winnerSS;
                winnerSS << "{\"type\":\"WINNER\",\"pots\":[";
                bool firstPot = true;
                
                for (size_t potIndex = 0; potIndex < sidePots.size(); ++potIndex) {
                    const auto& sidePot = sidePots[potIndex];
                    
                    if (sidePot.eligiblePlayers.empty()) {
                        // 沒有有資格的玩家（不應該發生）
                        continue;
                    }
                    
                    if (sidePot.eligiblePlayers.size() == 1) {
                        // 只有一個有資格的玩家，直接獲得
                        Player* winner = sidePot.eligiblePlayers[0];
                        winner->addChips(sidePot.amount);
                        if (potIndex == 0) winner->incrementHandsWon();
                        
                        std::string potName = (potIndex == 0) ? "Main Pot" : ("Side Pot " + std::to_string(potIndex));
                        LOG_INFO("Room", winner->getName() + " wins " + potName 
                                  + " ($" + std::to_string(sidePot.amount) + ") - no contest");
                        
                        if (!firstPot) winnerSS << ",";
                        firstPot = false;
                        winnerSS << "{\"pot\":\"" << potName << "\",\"amount\":" << sidePot.amount
                                 << ",\"winners\":[{\"player_id\":" << winner->getId()
                                 << ",\"name\":\"" << winner->getName() 
                                 << "\",\"amount\":" << sidePot.amount << "}]}";
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
                    
                    // 找出所有贏家（可能平手）
                    std::vector<Player*> potWinners;
                    const HandValue& bestHand = eligibleHands[0].second;
                    
                    for (const auto& eh : eligibleHands) {
                        if (eh.second == bestHand) {
                            potWinners.push_back(eh.first);
                        } else {
                            break;
                        }
                    }
                    
                    // 分配這個邊池
                    int share = sidePot.amount / static_cast<int>(potWinners.size());
                    int rem = sidePot.amount % static_cast<int>(potWinners.size());
                    
                    std::string potName = (potIndex == 0) ? "Main Pot" : ("Side Pot " + std::to_string(potIndex));
                    
                    if (!firstPot) winnerSS << ",";
                    firstPot = false;
                    winnerSS << "{\"pot\":\"" << potName << "\",\"amount\":" << sidePot.amount
                             << ",\"hand\":\"" << bestHand.toString() << "\",\"winners\":[";
                    
                    for (size_t i = 0; i < potWinners.size(); ++i) {
                        int award = share;
                        if (static_cast<int>(i) < rem) award++;
                        
                        potWinners[i]->addChips(award);
                        if (potIndex == 0 && i == 0) potWinners[i]->incrementHandsWon();
                        
                        if (potWinners.size() == 1) {
                            LOG_INFO("Room", potWinners[i]->getName() + " wins " + potName 
                                      + " ($" + std::to_string(award) + ") with " + bestHand.toString());
                        } else {
                            LOG_INFO("Room", potWinners[i]->getName() + " wins " + potName 
                                      + " ($" + std::to_string(award) + ", split) with " + bestHand.toString());
                        }
                        
                        if (i > 0) winnerSS << ",";
                        winnerSS << "{\"player_id\":" << potWinners[i]->getId()
                                 << ",\"name\":\"" << potWinners[i]->getName() 
                                 << ",\"amount\":" << award << "}";
                    }
                    winnerSS << "]}";
                }
                
                winnerSS << "]}";
                winnerMessage = winnerSS.str();
            }
        }
        
        // 更新所有玩家的遊戲統計
        for (auto& player : players) {
            if (player.isActive() || player.isAllIn() || player.isFolded()) {
                player.incrementHandsPlayed();
            }
        }
        
        // 構建 showdown 消息
        std::stringstream ss;
        ss << "{\"type\":\"SHOWDOWN\",\"players\":[";
        bool first = true;
        for (const auto& player : players) {
            if (!first) ss << ",";
            first = false;
            ss << formatPlayerJson(player, true);
        }
        ss << "],\"community_cards\":" << formatCardsJson(game_->getCommunityCards()) << "}";
        showdownMessage = ss.str();
        
        // 重置遊戲狀態
        gameInProgress_ = false;
        currentStage_ = GameStage::WAITING;
        
        // 檢查是否可以開始新遊戲
        if (autoStart_ && sessions_.size() >= MIN_PLAYERS) {
            shouldStartNewGame = true;
        }
        
        LOG_INFO("Room", "==================");
    }
    
    // 在鎖外發送
    broadcast(showdownMessage);
    if (!winnerMessage.empty()) {
        broadcast(winnerMessage);
    }
    broadcast("{\"type\":\"GAME_END\",\"message\":\"Hand complete\"}");
    
    // 自動開始新遊戲
    if (shouldStartNewGame) {
        LOG_INFO("Room", "Starting new game in " + std::to_string(AUTO_START_DELAY_MS / 1000) + " seconds...");
        
        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(AUTO_START_DELAY_MS));
            
            bool canStart = false;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                canStart = (autoStart_ && !gameInProgress_ && sessions_.size() >= MIN_PLAYERS);
            }
            
            if (canStart) {
                broadcast("{\"type\":\"NEW_HAND\",\"message\":\"Starting new hand...\"}");
                startGame();
            }
        }).detach();
    }
}

void Room::broadcast(const std::string& message, int excludePlayerId) {
    std::map<int, std::shared_ptr<Session>> sessionsCopy;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        sessionsCopy = sessions_;
    }
    
    for (const auto& [playerId, session] : sessionsCopy) {
        if (playerId != excludePlayerId) {
            try {
                session->send(message + "\n");
            } catch (const std::exception& e) {
                std::cerr << "Error broadcasting to player " << playerId 
                         << ": " << e.what() << std::endl;
            }
        }
    }
}

void Room::sendToPlayer(int playerId, const std::string& message) {
    std::shared_ptr<Session> session;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(playerId);
        if (it != sessions_.end()) {
            session = it->second;
        }
    }
    
    if (session) {
        try {
            session->send(message + "\n");
        } catch (const std::exception& e) {
            std::cerr << "Error sending to player " << playerId 
                     << ": " << e.what() << std::endl;
        }
    }
}

void Room::notifyGameState() {
    std::string state;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state = formatGameStateJsonUnsafe();
    }
    broadcast(state);
}

// ===============================
// JSON 格式化方法
// ===============================

std::string Room::formatCardJson(const Card& card) const {
    std::stringstream ss;
    ss << "{\"rank\":\"" << card.getRankString() << "\",\"suit\":\"" << card.getSuitString() 
       << "\",\"short\":\"" << card.toShortString() << "\"}";
    return ss.str();
}

std::string Room::formatCardsJson(const std::vector<Card>& cards) const {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < cards.size(); ++i) {
        if (i > 0) ss << ",";
        ss << formatCardJson(cards[i]);
    }
    ss << "]";
    return ss.str();
}

std::string Room::formatPlayerJson(const Player& player, bool includeHand) const {
    std::stringstream ss;
    ss << "{\"id\":" << player.getId()
       << ",\"name\":\"" << player.getName() << "\""
       << ",\"chips\":" << player.getChips()
       << ",\"current_bet\":" << player.getCurrentBet()
       << ",\"state\":" << static_cast<int>(player.getState())
       << ",\"is_dealer\":" << (player.isDealer() ? "true" : "false")
       << ",\"is_small_blind\":" << (player.isSmallBlind() ? "true" : "false")
       << ",\"is_big_blind\":" << (player.isBigBlind() ? "true" : "false");
    
    if (includeHand && !player.getHand().empty()) {
        ss << ",\"hand\":" << formatCardsJson(player.getHand());
    }
    
    ss << "}";
    return ss.str();
}

std::string Room::formatGameStateJsonUnsafe() const {
    std::stringstream ss;
    
    int currentPlayerId = -1;
    const auto& players = game_->getPlayers();
    if (currentPlayerIndex_ >= 0 && currentPlayerIndex_ < static_cast<int>(players.size())) {
        currentPlayerId = players[currentPlayerIndex_].getId();
    }
    
    ss << "{\"type\":\"GAME_STATE\""
       << ",\"pot\":" << game_->getPot()
       << ",\"current_bet\":" << game_->getCurrentBet()
       << ",\"stage\":\"" << stageToString(currentStage_) << "\""
       << ",\"current_player\":" << currentPlayerId
       << ",\"community_cards\":" << formatCardsJson(game_->getCommunityCards())
       << ",\"players\":[";
    
    for (size_t i = 0; i < players.size(); ++i) {
        if (i > 0) ss << ",";
        ss << formatPlayerJson(players[i], false);
    }
    
    ss << "]}";
    return ss.str();
}

std::string Room::stageToString(GameStage stage) const {
    switch (stage) {
        case GameStage::WAITING: return "waiting";
        case GameStage::PRE_FLOP: return "preflop";
        case GameStage::FLOP: return "flop";
        case GameStage::TURN: return "turn";
        case GameStage::RIVER: return "river";
        case GameStage::SHOWDOWN: return "showdown";
        default: return "unknown";
    }
}

// ===============================
// 舊格式（向後兼容）
// ===============================

std::string Room::formatGameStateUnsafe() const {
    std::stringstream ss;
    
    ss << game_->getPot() << "," << game_->getCurrentBet();
    
    const auto& communityCards = game_->getCommunityCards();
    if (!communityCards.empty()) {
        ss << ",";
        for (size_t i = 0; i < communityCards.size(); ++i) {
            if (i > 0) ss << ";";
            ss << communityCards[i].toString();
        }
    }
    
    const auto& players = game_->getPlayers();
    for (const auto& player : players) {
        ss << "|" << player.getId() << "," << player.getName() 
           << "," << player.getChips() << "," << static_cast<int>(player.getState());
    }
    
    return ss.str();
}

std::string Room::getRoomState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::stringstream ss;
    ss << "Room " << roomId_ << " - Players: " << sessions_.size() 
       << "/" << MAX_PLAYERS << ", Game: " << (gameInProgress_ ? "In Progress" : "Waiting");
    return ss.str();
}

std::string Room::getRoomStateJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::stringstream ss;
    ss << "{\"type\":\"ROOM_STATE\",\"room_id\":" << roomId_ 
       << ",\"player_count\":" << sessions_.size()
       << ",\"max_players\":" << MAX_PLAYERS
       << ",\"game_in_progress\":" << (gameInProgress_ ? "true" : "false")
       << ",\"stage\":\"" << stageToString(currentStage_) << "\"}";
    return ss.str();
}

std::string Room::getGameState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return formatGameStateUnsafe();
}

std::string Room::getGameStateJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return formatGameStateJsonUnsafe();
}

std::string Room::getPlayerList() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return getPlayerListUnsafe();
}

std::string Room::getPlayerListJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::stringstream ss;
    ss << "{\"type\":\"PLAYERS\",\"players\":" + getPlayerListUnsafe() + "}";
    return ss.str();
}

std::string Room::getPlayerListUnsafe() const {
    std::stringstream ss;
    ss << "[";
    const auto& players = game_->getPlayers();
    
    for (size_t i = 0; i < players.size(); ++i) {
        if (i > 0) ss << ",";
        ss << formatPlayerJson(players[i], false);
    }
    
    ss << "]";
    return ss.str();
}
