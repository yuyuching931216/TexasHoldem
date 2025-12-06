#include "Room.h"
#include "Session.h"
#include "HandEvaluator.h"
#include <sstream>
#include <iostream>
#include <algorithm>

Room::Room(int roomId)
    : roomId_(roomId), game_(std::make_unique<PokerGame>()), 
      nextPlayerId_(1), gameInProgress_(false) {
    std::cout << "Room " << roomId_ << " created" << std::endl;
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
        
        std::stringstream ss;
        ss << "JOINED|" << playerId << "|" << playerName << "|" << buyIn;
        joinMessage = ss.str();
        
        playerListMessage = "PLAYERS|" + getPlayerListUnsafe();
        
        std::cout << "Player " << playerName << " (ID: " << playerId << ") joined room " 
                  << roomId_ << std::endl;
    }
    
    // 在鎖外發送消息
    broadcast(joinMessage);
    sendToPlayer(playerId, playerListMessage);
    
    return true;
}

void Room::removePlayer(int playerId) {
    std::string message;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = sessions_.find(playerId);
        if (it != sessions_.end()) {
            sessions_.erase(it);
            
            std::stringstream ss;
            ss << "LEFT|" << playerId;
            message = ss.str();
            
            std::cout << "Player " << playerId << " left room " << roomId_ << std::endl;
        }
    }
    
    if (!message.empty()) {
        broadcast(message);
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
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        canStart = (sessions_.size() >= MIN_PLAYERS && !gameInProgress_);
        
        if (canStart) {
            gameInProgress_ = true;
        }
    }
    
    if (!canStart) {
        broadcast("ERROR|Not enough players to start game");
        return;
    }
    
    broadcast("GAME_START|Game is starting...");
    
    try {
        notifyGameState();
    } catch (const std::exception& e) {
        std::cerr << "Game error: " << e.what() << std::endl;
        broadcast("ERROR|" + std::string(e.what()));
        
        std::lock_guard<std::mutex> lock(mutex_);
        gameInProgress_ = false;
    }
}

void Room::processPlayerAction(int playerId, const std::string& action, int amount) {
    bool inProgress;
    std::string message;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        inProgress = gameInProgress_;
        
        if (inProgress) {
            std::stringstream ss;
            ss << "ACTION|" << playerId << "|" << action;
            if (amount > 0) {
                ss << "|" << amount;
            }
            message = ss.str();
        }
    }
    
    if (!inProgress) {
        sendToPlayer(playerId, "ERROR|No game in progress");
        return;
    }
    
    broadcast(message);
    notifyGameState();
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
    std::string state = formatGameState();
    broadcast("STATE|" + state);
}

std::string Room::formatGameState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return formatGameStateUnsafe();
}

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

std::string Room::getPlayerList() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return getPlayerListUnsafe();
}

std::string Room::getPlayerListUnsafe() const {
    std::stringstream ss;
    const auto& players = game_->getPlayers();
    
    for (size_t i = 0; i < players.size(); ++i) {
        if (i > 0) ss << "|";
        ss << players[i].getId() << "," << players[i].getName() 
           << "," << players[i].getChips();
    }
    
    return ss.str();
}
