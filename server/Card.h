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
    std::string toShortString() const;  // 短格式: "2h", "As"
    std::string getRankString() const;  // "2", "10", "J", "Q", "K", "A"
    std::string getSuitString() const;  // "hearts", "diamonds", "clubs", "spades"
    char getRankChar() const;           // '2'-'9', 'T', 'J', 'Q', 'K', 'A'
    char getSuitChar() const;           // 'h', 'd', 'c', 's'
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