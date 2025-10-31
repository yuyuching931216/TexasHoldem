#include <vector>
#include <string>
#include "Card.h"

class PokerGame {
    public:
        PokerGame();
        void startGame();
        void dealHoleCards();
        void dealFlop();
        void dealTurn();
        void dealRiver();
        std::string getGameState() const;
        
    private:
        Deck deck_;
        std::vector<Card> communityCards_;
        std::vector<std::vector<Card>> playerHands_;
        int currentPlayer_;
        int pot_;
    };