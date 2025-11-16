#include "Game.h"
#include "Player.h"
#include <iostream>
#include <memory>
#include <vector>

int aabbmain() {
    std::cout << "=== Smart Pointer Texas Hold'em Test ===" << std::endl;
    
    // 測試1：傳統方式（保持原有簡潔性）
    {
        std::cout << "\n--- Test 1: Traditional Player Objects ---" << std::endl;
        PokerGame traditionalGame;
        
        Player player1(1, "Alice", 1000);
        Player player2(2, "Bob", 1000);
        
        traditionalGame.addPlayer(player1);
        traditionalGame.addPlayer(player2);
        
        traditionalGame.startGame();
    }
    
    // 測試2：展示智能指針在玩家管理中的應用
    {
        std::cout << "\n\n--- Test 2: Smart Pointer for Player Management ---" << std::endl;
        
        // 使用智能指針創建玩家池
        std::vector<std::unique_ptr<Player>> playerPool;
        
        // 創建玩家
        playerPool.push_back(std::make_unique<Player>(1, "Charlie", 1500));
        playerPool.push_back(std::make_unique<Player>(2, "Diana", 1200));
        playerPool.push_back(std::make_unique<Player>(3, "Eve", 1800));
        
        std::cout << "Created player pool with " << playerPool.size() << " players:" << std::endl;
        for (const auto& player : playerPool) {
            std::cout << "  - " << player->getName() << " ($" << player->getChips() << ")" << std::endl;
        }
        
        // 傳統遊戲仍然使用 vector<Player>，但我們可以從智能指針轉換
        PokerGame game;
        
        for (const auto& player : playerPool) {
            // 解引用智能指針並添加到遊戲
            game.addPlayer(*player);
        }
        
        std::cout << "\nStarting game..." << std::endl;
        game.startGame();
    }
    
    // 測試3：展示智能指針的內存安全優勢
    {
        std::cout << "\n\n--- Test 3: Memory Safety and RAII ---" << std::endl;
        
        // 展示自動內存管理
        {
            std::cout << "Creating scoped player objects..." << std::endl;
            auto player1 = std::make_unique<Player>(1, "Frank", 2000);
            auto player2 = std::make_unique<Player>(2, "Grace", 2500);
            
            std::cout << "Player1: " << player1->getName() << " has $" << player1->getChips() << std::endl;
            std::cout << "Player2: " << player2->getName() << " has $" << player2->getChips() << std::endl;
            
            // 轉移所有權演示
            std::vector<std::unique_ptr<Player>> tempStorage;
            std::cout << "\nTransferring ownership to temporary storage..." << std::endl;
            
            tempStorage.push_back(std::move(player1));
            tempStorage.push_back(std::move(player2));
            
            // 原指針現在為空
            std::cout << "Original pointers after move:" << std::endl;
            std::cout << "  player1: " << (player1 ? "Still valid" : "Moved") << std::endl;
            std::cout << "  player2: " << (player2 ? "Still valid" : "Moved") << std::endl;
            
            // 從存儲中訪問
            std::cout << "\nAccessing from storage:" << std::endl;
            for (const auto& player : tempStorage) {
                std::cout << "  - " << player->getName() << " ($" << player->getChips() << ")" << std::endl;
            }
            
            std::cout << "Exiting scope - players will be automatically destroyed..." << std::endl;
        } // 智能指針在此處自動釋放內存
        
        std::cout << "Memory automatically cleaned up!" << std::endl;
    }
    
    // 測試4：展示 shared_ptr 的共享所有權
    {
        std::cout << "\n\n--- Test 4: Shared Ownership with shared_ptr ---" << std::endl;
        
        // 創建一個共享的玩家對象
        auto sharedPlayer = std::make_shared<Player>(1, "SharedHenry", 3000);
        
        std::cout << "Initial reference count: " << sharedPlayer.use_count() << std::endl;
        
        // 多個觀察者共享同一個玩家
        std::vector<std::shared_ptr<Player>> observers;
        std::vector<std::shared_ptr<Player>> managers;
        
        observers.push_back(sharedPlayer);
        observers.push_back(sharedPlayer);
        managers.push_back(sharedPlayer);
        
        std::cout << "Reference count after sharing: " << sharedPlayer.use_count() << std::endl;
        
        // 展示共享訪問
        std::cout << "All references point to the same player:" << std::endl;
        std::cout << "  Original: " << sharedPlayer->getName() << " ($" << sharedPlayer->getChips() << ")" << std::endl;
        std::cout << "  Observer1: " << observers[0]->getName() << " ($" << observers[0]->getChips() << ")" << std::endl;
        std::cout << "  Manager: " << managers[0]->getName() << " ($" << managers[0]->getChips() << ")" << std::endl;
        
        // 修改一個引用會影響所有引用
        sharedPlayer->addChips(500);
        std::cout << "\nAfter adding $500 through original reference:" << std::endl;
        std::cout << "  Observer sees: $" << observers[0]->getChips() << std::endl;
        std::cout << "  Manager sees: $" << managers[0]->getChips() << std::endl;
        
        // 清除部分引用
        observers.clear();
        std::cout << "\nReference count after clearing observers: " << sharedPlayer.use_count() << std::endl;
        
        managers.clear();
        std::cout << "Reference count after clearing managers: " << sharedPlayer.use_count() << std::endl;
    }
    
    // 測試5：實際遊戲中的應用
    {
        std::cout << "\n\n--- Test 5: Practical Game Application ---" << std::endl;
        
        // 創建玩家工廠函數（使用智能指針）
        auto createPlayer = [](int id, const std::string& name, int chips) -> std::unique_ptr<Player> {
            return std::make_unique<Player>(id, name, chips);
        };
        
        // 批量創建玩家
        std::vector<std::unique_ptr<Player>> playerFactory;
        playerFactory.push_back(createPlayer(1, "SmartAlice", 1000));
        playerFactory.push_back(createPlayer(2, "SmartBob", 1500));
        playerFactory.push_back(createPlayer(3, "SmartCharlie", 1200));
        
        // 將智能指針管理的玩家添加到遊戲
        PokerGame smartGame;
        for (const auto& player : playerFactory) {
            smartGame.addPlayer(*player); // 解引用傳遞給遊戲
        }
        
        std::cout << "Starting game with smart pointer managed players..." << std::endl;
        smartGame.startGame();
        
        std::cout << "Players automatically cleaned up when going out of scope." << std::endl;
    }
    
    std::cout << "\n=== All Smart Pointer Tests Completed Successfully! ===" << std::endl;
    
    return 0;
}