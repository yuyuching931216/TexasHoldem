#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Card.h"

enum class PlayerAction {
    FOLD,
    CHECK,
    CALL,
    RAISE,
    ALL_IN,
    WAIT
};

enum class PlayerState {
    ACTIVE,
    FOLDED,
    ALL_IN,
    DISCONNECTED,
    WAITING
};

class Player {
public:
    // Constructor
    Player(int playerId, const std::string& name, int chips = 1000);
    
    // Destructor
    ~Player() = default;
    
    // Copy constructor and assignment operator
    Player(const Player& other) = default;
    Player& operator=(const Player& other) = default;
    
    // Basic player information
    int getId() const { return playerId_; }
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }
    
    // Chip management
    int getChips() const { return chips_; }
    void addChips(int amount);
    bool removeChips(int amount);
    void setChips(int chips) { chips_ = chips; }
    
    // Hand management
    void receiveCard(const Card& card);
    void clearHand();
    const std::vector<Card>& getHand() const { return hand_; }
    std::string getHandString() const;
    
    // Betting actions
    PlayerAction getLastAction() const { return lastAction_; }
    int getLastBet() const { return lastBet_; }
    int getCurrentBet() const { return currentBet_; }
    void setCurrentBet(int bet) { currentBet_ = bet; }
    
    // Action methods
    bool fold();
    bool check();
    bool call(int callAmount);
    bool raise(int raiseAmount);
    bool allIn();
    
    // Player state management
    PlayerState getState() const { return state_; }
    void setState(PlayerState state) { state_ = state; }
    bool isActive() const { return state_ == PlayerState::ACTIVE; }
    bool isFolded() const { return state_ == PlayerState::FOLDED; }
    bool isAllIn() const { return state_ == PlayerState::ALL_IN; }
    
    // Network connection management
    void setConnectionId(int connectionId) { connectionId_ = connectionId; }
    int getConnectionId() const { return connectionId_; }
    void setConnected(bool connected) { isConnected_ = connected; }
    bool isConnected() const { return isConnected_; }
    
    // Position management
    void setPosition(int position) { position_ = position; }
    int getPosition() const { return position_; }
    bool isDealer() const { return isDealer_; }
    void setDealer(bool dealer) { isDealer_ = dealer; }
    bool isSmallBlind() const { return isSmallBlind_; }
    void setSmallBlind(bool smallBlind) { isSmallBlind_ = smallBlind; }
    bool isBigBlind() const { return isBigBlind_; }
    void setBigBlind(bool bigBlind) { isBigBlind_ = bigBlind; }
    
    // Round management
    void startNewRound();
    void resetForNewHand();
    
    // Statistics
    int getHandsPlayed() const { return handsPlayed_; }
    int getHandsWon() const { return handsWon_; }
    void incrementHandsPlayed() { handsPlayed_++; }
    void incrementHandsWon() { handsWon_++; }
    double getWinRate() const;
    
    // Utility methods
    std::string toString() const;
    std::string getStatusString() const;
    bool canAct() const;
    bool hasEnoughChips(int amount) const;

private:
    // Player identification
    int playerId_;
    std::string name_;
    
    // Game state
    int chips_;
    std::vector<Card> hand_;
    PlayerState state_;
    PlayerAction lastAction_;
    int lastBet_;
    int currentBet_;
    
    // Position information
    int position_;
    bool isDealer_;
    bool isSmallBlind_;
    bool isBigBlind_;
    
    // Network information
    int connectionId_;
    bool isConnected_;
    
    // Statistics
    int handsPlayed_;
    int handsWon_;
};