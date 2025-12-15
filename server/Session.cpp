#include "Session.h"
#include "Room.h"
#include <iostream>
#include <sstream>

Session::Session(boost::asio::ip::tcp::socket socket, std::shared_ptr<Room> room)
    : socket_(std::move(socket)), room_(room), playerId_(-1) {
}

void Session::start() {
    doRead();
}

void Session::send(const std::string& message) {
    auto self(shared_from_this());
    boost::asio::post(socket_.get_executor(),
        [this, self, message]() {
            bool writeInProgress = !writeQueue_.empty();
            writeQueue_.push_back(message);
            if (!writeInProgress) {
                doWrite();
            }
        });
}

void Session::doRead() {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(readBuffer_),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                std::string message(readBuffer_.data(), length);
                
                // 處理可能包含多個命令的情況（用換行符分隔）
                std::stringstream ss(message);
                std::string line;
                while (std::getline(ss, line)) {
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }
                    if (!line.empty()) {
                        processMessage(line);
                    }
                }
                
                doRead();
            } else {
                std::cout << "Client disconnected: " << ec.message() << std::endl;
                if (playerId_ != -1 && room_) {
                    room_->removePlayer(playerId_);
                }
            }
        });
}

void Session::doWrite() {
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
        boost::asio::buffer(writeQueue_.front().data(), writeQueue_.front().length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                writeQueue_.pop_front();
                if (!writeQueue_.empty()) {
                    doWrite();
                }
            } else {
                std::cerr << "Write error: " << ec.message() << std::endl;
                if (playerId_ != -1 && room_) {
                    room_->removePlayer(playerId_);
                }
            }
        });
}

void Session::processMessage(const std::string& message) {
    std::cout << "Received message: " << message << std::endl;
    
    // 嘗試解析為 JSON 格式
    if (message.front() == '{') {
        processJsonMessage(message);
        return;
    }
    
    // 文本格式命令
    std::istringstream iss(message);
    std::string command;
    iss >> command;
    
    try {
        if (command == "JOIN") {
            // JOIN <playerName> [buyIn]
            std::string playerName;
            int buyIn = 1000;
            
            iss >> playerName;
            if (iss >> buyIn) {
                // 買入金額已提供
            }
            
            if (room_->addPlayer(shared_from_this(), playerName, buyIn)) {
                send("{\"type\":\"OK\",\"message\":\"Joined room as " + playerName + "\"}\n");
            } else {
                send("{\"type\":\"ERROR\",\"message\":\"Room is full\"}\n");
            }
        }
        else if (command == "START") {
            // START - 開始遊戲
            if (room_->canStartGame()) {
                room_->startGame();
            } else {
                send("{\"type\":\"ERROR\",\"message\":\"Cannot start game (need 2+ players)\"}\n");
            }
        }
        else if (command == "ACTION") {
            // ACTION <action> [amount]
            // action: FOLD, CHECK, CALL, RAISE, ALL_IN
            std::string action;
            int amount = 0;
            
            iss >> action;
            if (iss >> amount) {
                // 金額已提供
            }
            
            room_->processPlayerAction(playerId_, action, amount);
        }
        else if (command == "STATUS") {
            // STATUS - 查詢房間基本狀態（JSON 格式）
            send(room_->getRoomStateJson() + "\n");
        }
        else if (command == "GAMESTATE") {
            // GAMESTATE - 查詢詳細遊戲狀態（JSON 格式）
            send(room_->getGameStateJson() + "\n");
        }
        else if (command == "PLAYERS") {
            // PLAYERS - 查詢玩家列表（JSON 格式）
            send(room_->getPlayerListJson() + "\n");
        }
        else if (command == "QUIT") {
            // QUIT - 離開房間
            send("{\"type\":\"BYE\",\"message\":\"Goodbye\"}\n");
            if (playerId_ != -1) {
                room_->removePlayer(playerId_);
            }
            socket_.close();
        }
        else {
            send("{\"type\":\"ERROR\",\"message\":\"Unknown command: " + command + "\"}\n");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing message: " << e.what() << std::endl;
        send("{\"type\":\"ERROR\",\"message\":\"" + std::string(e.what()) + "\"}\n");
    }
}

void Session::processJsonMessage(const std::string& json) {
    // 簡單的 JSON 解析（不使用外部庫）
    try {
        // 尋找 "action" 字段
        size_t actionPos = json.find("\"action\"");
        if (actionPos != std::string::npos) {
            // 提取 action 值
            size_t colonPos = json.find(':', actionPos);
            size_t quoteStart = json.find('"', colonPos);
            size_t quoteEnd = json.find('"', quoteStart + 1);
            
            if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                std::string action = json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                
                // 尋找 amount
                int amount = 0;
                size_t amountPos = json.find("\"amount\"");
                if (amountPos != std::string::npos) {
                    size_t colonPos2 = json.find(':', amountPos);
                    size_t numStart = colonPos2 + 1;
                    while (numStart < json.size() && (json[numStart] == ' ' || json[numStart] == '\t')) {
                        numStart++;
                    }
                    size_t numEnd = numStart;
                    while (numEnd < json.size() && (json[numEnd] >= '0' && json[numEnd] <= '9')) {
                        numEnd++;
                    }
                    if (numEnd > numStart) {
                        amount = std::stoi(json.substr(numStart, numEnd - numStart));
                    }
                }
                
                room_->processPlayerAction(playerId_, action, amount);
                return;
            }
        }
        
        // 尋找 "command" 字段
        size_t cmdPos = json.find("\"command\"");
        if (cmdPos != std::string::npos) {
            size_t colonPos = json.find(':', cmdPos);
            size_t quoteStart = json.find('"', colonPos);
            size_t quoteEnd = json.find('"', quoteStart + 1);
            
            if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                std::string command = json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                
                if (command == "JOIN") {
                    // 尋找 name
                    std::string name = "Player";
                    int buyIn = 1000;
                    
                    size_t namePos = json.find("\"name\"");
                    if (namePos != std::string::npos) {
                        size_t colonPos2 = json.find(':', namePos);
                        size_t quoteStart2 = json.find('"', colonPos2);
                        size_t quoteEnd2 = json.find('"', quoteStart2 + 1);
                        if (quoteStart2 != std::string::npos && quoteEnd2 != std::string::npos) {
                            name = json.substr(quoteStart2 + 1, quoteEnd2 - quoteStart2 - 1);
                        }
                    }
                    
                    size_t buyInPos = json.find("\"buy_in\"");
                    if (buyInPos != std::string::npos) {
                        size_t colonPos2 = json.find(':', buyInPos);
                        size_t numStart = colonPos2 + 1;
                        while (numStart < json.size() && (json[numStart] == ' ' || json[numStart] == '\t')) {
                            numStart++;
                        }
                        size_t numEnd = numStart;
                        while (numEnd < json.size() && (json[numEnd] >= '0' && json[numEnd] <= '9')) {
                            numEnd++;
                        }
                        if (numEnd > numStart) {
                            buyIn = std::stoi(json.substr(numStart, numEnd - numStart));
                        }
                    }
                    
                    if (room_->addPlayer(shared_from_this(), name, buyIn)) {
                        send("{\"type\":\"OK\",\"message\":\"Joined room as " + name + "\"}\n");
                    } else {
                        send("{\"type\":\"ERROR\",\"message\":\"Room is full\"}\n");
                    }
                }
                else if (command == "START") {
                    if (room_->canStartGame()) {
                        room_->startGame();
                    } else {
                        send("{\"type\":\"ERROR\",\"message\":\"Cannot start game\"}\n");
                    }
                }
                else if (command == "STATUS") {
                    send(room_->getRoomStateJson() + "\n");
                }
                else if (command == "GAMESTATE") {
                    send(room_->getGameStateJson() + "\n");
                }
                else if (command == "QUIT") {
                    send("{\"type\":\"BYE\",\"message\":\"Goodbye\"}\n");
                    if (playerId_ != -1) {
                        room_->removePlayer(playerId_);
                    }
                    socket_.close();
                }
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        send("{\"type\":\"ERROR\",\"message\":\"Invalid JSON format\"}\n");
    }
}
