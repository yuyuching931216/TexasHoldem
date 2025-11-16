#include "Card.h"
#include "Player.h"
#include "Game.h"
#include <iostream>
#include <vector>

int main() {
    std::cout << "Texas Hold'em Server Starting..." << std::endl;
    
    // 創建簡單的測試
    PokerGame game;
    
    // 添加玩家
    for (int i = 1; i <= 8; ++i) {
        game.addPlayer(Player(i, "Player" + std::to_string(i), 1000));
    }

    // 開始遊戲
    game.startGame();
    
    //std::cout << "Game State:" << std::endl;
    //std::cout << game.getGameState() << std::endl;
    
    return 0;
}