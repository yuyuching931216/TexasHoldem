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
                send("OK|Joined room as " + playerName + "\n");
            } else {
                send("ERROR|Room is full\n");
            }
        }
        else if (command == "START") {
            // START - 開始遊戲
            if (room_->canStartGame()) {
                room_->startGame();
            } else {
                send("ERROR|Cannot start game (need 2+ players)\n");
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
            // STATUS - 查詢房間狀態
            send("STATUS|" + room_->getRoomState() + "\n");
        }
        else if (command == "QUIT") {
            // QUIT - 離開房間
            send("BYE|Goodbye\n");
            if (playerId_ != -1) {
                room_->removePlayer(playerId_);
            }
            socket_.close();
        }
        else {
            send("ERROR|Unknown command: " + command + "\n");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing message: " << e.what() << std::endl;
        send("ERROR|" + std::string(e.what()) + "\n");
    }
}
