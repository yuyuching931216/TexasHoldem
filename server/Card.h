#pragma once
#include <vector>
#include <string>

enum class Suit { HEARTS, DIAMONDS, CLUBS, SPADES };
enum class Rank { TWO=2, THREE, FOUR, FIVE, SIX, SEVEN, 
                  EIGHT, NINE, TEN, JACK, QUEEN, KING, ACE };

class Card {
public:
    Card(Suit suit, Rank rank);
    std::string toString() const;
    Suit getSuit() const { return suit_; }
    Rank getRank() const { return rank_; }
    
private:
    Suit suit_;
    Rank rank_;
};

class Deck {
public:
    Deck();
    ~Deck() = default;  // 使用預設解構子
    void shuffle();
    Card dealCard();
    bool isEmpty() const;
	void reset();
    
private:
    std::vector<Card> cards_;
    size_t currentCard_;
};