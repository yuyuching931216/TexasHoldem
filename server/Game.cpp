#include "Game.h"
#include "Player.h"
#include <iostream>
#include <stdexcept>
#include <random>
#include <algorithm>

// ───────────────────────────────
// 建構子
// ───────────────────────────────
PokerGame::PokerGame()
    : currentPlayer_(0), pot_(0)
{
    // 假設有 4 位玩家，起始籌碼 1000
    for (int i = 0; i < 4; ++i) {
        std::string name = "Player" + std::to_string(i + 1);
        players_.emplace_back(i, name, 1000);
    }
}

// ───────────────────────────────
// 遊戲開始
// ───────────────────────────────
void PokerGame::startGame() {
    deck_.shuffle();
    dealHoleCards();
    dealFlop();
    dealTurn();
    dealRiver();

    // 顯示遊戲狀態
    std::cout << getGameState() << std::endl;
}

// ───────────────────────────────
// 發底牌 (每位玩家兩張)
// ───────────────────────────────
void PokerGame::dealHoleCards() {
    for (int i = 0; i < 2; ++i) {
        for (auto& player : players_) {
            if (deck_.isEmpty()) throw std::out_of_range("Deck is empty!");
            player.receiveCard(deck_.dealCard());
        }
    }
}

// ───────────────────────────────
// 發公共牌 (Flop)
// ───────────────────────────────
void PokerGame::dealFlop() {
    for (int i = 0; i < 3; ++i) {
        if (deck_.isEmpty()) throw std::out_of_range("Deck is empty!");
        communityCards_.push_back(deck_.dealCard());
    }
}

// ───────────────────────────────
// 發 Turn (轉牌)
// ───────────────────────────────
void PokerGame::dealTurn() {
    if (deck_.isEmpty()) throw std::out_of_range("Deck is empty!");
    communityCards_.push_back(deck_.dealCard());
}

// ───────────────────────────────
// 發 River (河牌)
// ───────────────────────────────
void PokerGame::dealRiver() {
    if (deck_.isEmpty()) throw std::out_of_range("Deck is empty!");
    communityCards_.push_back(deck_.dealCard());
}

// ───────────────────────────────
// 顯示遊戲狀態
// ───────────────────────────────
std::string PokerGame::getGameState() const {
    std::string state;

    // 玩家手牌
    for (const auto& player : players_) {
        state += player.toString() + "\n";
    }

    // 公共牌
    state += "\nCommunity Cards:\n";
    for (const auto& card : communityCards_) {
        state += "  " + card.toString() + "\n";
    }

    return state;
}

