#include "Card.h"
#include <algorithm>
#include <random>
#include <stdexcept>


Card::Card(Suit suit, Rank rank) : suit_(suit), rank_(rank) {}

std::string Card::toString() const {
    static const char* suitNames[] = {"Hearts", "Diamonds", "Clubs", "Spades"};
    static const char* rankNames[] = {
        "2", "3", "4", "5", "6", "7", "8", "9", "10",
        "Jack", "Queen", "King", "Ace"
    };
    return std::string(rankNames[static_cast<int>(rank_) - 2]) + " of " + suitNames[static_cast<int>(suit_)];
}

std::string Card::toShortString() const {
    std::string result;
    result += getRankChar();
    result += getSuitChar();
    return result;
}

std::string Card::getRankString() const {
    static const char* rankNames[] = {
        "2", "3", "4", "5", "6", "7", "8", "9", "10",
        "J", "Q", "K", "A"
    };
    return rankNames[static_cast<int>(rank_) - 2];
}

std::string Card::getSuitString() const {
    static const char* suitNames[] = {"hearts", "diamonds", "clubs", "spades"};
    return suitNames[static_cast<int>(suit_)];
}

char Card::getRankChar() const {
    static const char rankChars[] = "23456789TJQKA";
    int index = static_cast<int>(rank_) - 2;
    if (index >= 0 && index < 13) {
        return rankChars[index];
    }
    return '?';
}

char Card::getSuitChar() const {
    static const char suitChars[] = "hdcs";
    int index = static_cast<int>(suit_);
    if (index >= 0 && index < 4) {
        return suitChars[index];
    }
    return '?';
}

Deck::Deck() : currentCard_(0) {
    for (int s = 0; s < 4; ++s) {
        for (int r = 2; r <= 14; ++r) {
            cards_.emplace_back(static_cast<Suit>(s), static_cast<Rank>(r));
        }
    }
    shuffle();
}

void Deck::shuffle() {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(cards_.begin(), cards_.end(), g);
    currentCard_ = 0;
}

Card Deck::dealCard() {
    if (isEmpty()) {
        throw std::out_of_range("No more cards in the deck");
    }
    return cards_[currentCard_++];
}

bool Deck::isEmpty() const {
    return currentCard_ >= cards_.size();
}

void Deck::reset() {
    currentCard_ = 0;
    shuffle();
}