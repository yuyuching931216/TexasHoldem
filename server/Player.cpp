#include "Player.h"
#include <sstream>
#include <algorithm>

// Constructor
Player::Player(int playerId, const std::string& name, int chips)
    : playerId_(playerId), name_(name), chips_(chips), state_(PlayerState::WAITING),
      lastAction_(PlayerAction::WAIT), lastBet_(0), currentBet_(0), position_(-1),
      isDealer_(false), isSmallBlind_(false), isBigBlind_(false),
      connectionId_(-1), isConnected_(false), handsPlayed_(0), handsWon_(0) {
    hand_.reserve(2); // Texas Hold'em uses 2 hole cards
}

// Chip management
void Player::addChips(int amount) {
    if (amount > 0) {
        chips_ += amount;
    }
}

bool Player::removeChips(int amount) {
    if (amount > 0 && chips_ >= amount) {
        chips_ -= amount;
        return true;
    }
    return false;
}

// Hand management
void Player::receiveCard(const Card& card) {
    if (hand_.size() < 2) {  // Limit to 2 hole cards in Texas Hold'em
        hand_.push_back(card);
    }
}

void Player::clearHand() {
    hand_.clear();
}

std::string Player::getHandString() const {
    std::stringstream ss;
    for (size_t i = 0; i < hand_.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << hand_[i].toString();
    }
    return ss.str();
}

// Action methods
bool Player::fold() {
    if (state_ == PlayerState::ACTIVE) {
        state_ = PlayerState::FOLDED;
        lastAction_ = PlayerAction::FOLD;
        return true;
    }
    return false;
}

bool Player::check() {
    // 只有在活躍狀態且沒有需要跟注的情況下才能 check
    // 這個方法現在主要由 Game 類別來控制邏輯
    if (state_ == PlayerState::ACTIVE) {
        lastAction_ = PlayerAction::CHECK;
        return true;
    }
    return false;
}

bool Player::call(int callAmount) {
    if (state_ != PlayerState::ACTIVE || callAmount < 0) {
        return false;
    }
    
    int amountToCall = callAmount - currentBet_;
    
    if (amountToCall <= 0) {
        // 如果沒有需要額外支付的金額，這應該是 check
        return check();
    }
    
    if (chips_ >= amountToCall) {
        removeChips(amountToCall);
        lastBet_ = amountToCall;
        currentBet_ = callAmount;
        lastAction_ = PlayerAction::CALL;
        return true;
    } else if (chips_ > 0) {
        // All-in call
        return allIn();
    }
    
    return false;
}

bool Player::raise(int totalBetAmount) {
    if (state_ != PlayerState::ACTIVE || totalBetAmount <= currentBet_) {
        return false;
    }
    
    int amountToRaise = totalBetAmount - currentBet_;
    
    if (chips_ >= amountToRaise) {
        removeChips(amountToRaise);
        lastBet_ = amountToRaise;
        currentBet_ = totalBetAmount;
        lastAction_ = PlayerAction::RAISE;
        return true;
    } else if (chips_ > 0) {
        // All-in raise
        return allIn();
    }
    
    return false;
}

bool Player::allIn() {
    if (state_ != PlayerState::ACTIVE || chips_ == 0) {
        return false;
    }
    
    lastBet_ = chips_;
    currentBet_ += chips_;
    chips_ = 0;
    state_ = PlayerState::ALL_IN;
    lastAction_ = PlayerAction::ALL_IN;
    return true;
}

// Round management  
void Player::startNewRound() {
    lastAction_ = PlayerAction::WAIT;
    lastBet_ = 0;
    // 注意：不重置 currentBet_，因為這在整個下注回合中需要保持
    
    if (state_ != PlayerState::DISCONNECTED && state_ != PlayerState::FOLDED && chips_ > 0) {
        state_ = PlayerState::ACTIVE;
    } else if (chips_ == 0) {
        state_ = PlayerState::ALL_IN;
    }
}

void Player::resetForNewHand() {
    clearHand();
    state_ = (chips_ > 0 && isConnected_) ? PlayerState::WAITING : PlayerState::DISCONNECTED;
    lastAction_ = PlayerAction::WAIT;
    lastBet_ = 0;
    currentBet_ = 0;
    isDealer_ = false;
    isSmallBlind_ = false;
    isBigBlind_ = false;
}

// Statistics
double Player::getWinRate() const {
    if (handsPlayed_ == 0) {
        return 0.0;
    }
    return static_cast<double>(handsWon_) / static_cast<double>(handsPlayed_) * 100.0;
}

// Utility methods
std::string Player::toString() const {
    std::stringstream ss;
    ss << "Player " << playerId_ << " (" << name_ << "): ";
    ss << "Chips=" << chips_ << ", ";
    ss << "State=" << static_cast<int>(state_) << ", ";
    ss << "Position=" << position_;
    
    if (!hand_.empty()) {
        ss << ", Hand=[" << getHandString() << "]";
    }
    
    return ss.str();
}

std::string Player::getStatusString() const {
    std::stringstream ss;
    ss << name_ << " ($" << chips_ << ")";
    
    switch (state_) {
        case PlayerState::ACTIVE:
            ss << " [Active]";
            break;
        case PlayerState::FOLDED:
            ss << " [Folded]";
            break;
        case PlayerState::ALL_IN:
            ss << " [All-in]";
            break;
        case PlayerState::DISCONNECTED:
            ss << " [Disconnected]";
            break;
        case PlayerState::WAITING:
            ss << " [Waiting]";
            break;
    }
    
    if (isDealer_) ss << " [D]";
    if (isSmallBlind_) ss << " [SB]";
    if (isBigBlind_) ss << " [BB]";
    
    if (lastAction_ != PlayerAction::WAIT) {
        ss << " Last: ";
        switch (lastAction_) {
            case PlayerAction::FOLD:
                ss << "Fold";
                break;
            case PlayerAction::CHECK:
                ss << "Check";
                break;
            case PlayerAction::CALL:
                ss << "Call $" << lastBet_;
                break;
            case PlayerAction::RAISE:
                ss << "Raise $" << lastBet_;
                break;
            case PlayerAction::ALL_IN:
                ss << "All-in $" << lastBet_;
                break;
            default:
                break;
        }
    }
    
    return ss.str();
}

bool Player::canAct() const {
    return (state_ == PlayerState::ACTIVE) && isConnected_;
}

bool Player::hasEnoughChips(int amount) const {
    return chips_ >= amount;
}

