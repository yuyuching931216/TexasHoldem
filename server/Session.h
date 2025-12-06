#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <deque>

class Room;

// Session類別 - 管理單個客戶端連接
class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::ip::tcp::socket socket, std::shared_ptr<Room> room);
    
    void start();
    void send(const std::string& message);
    int getPlayerId() const { return playerId_; }
    void setPlayerId(int id) { playerId_ = id; }
    
private:
    void doRead();
    void doWrite();
    void processMessage(const std::string& message);
    
    boost::asio::ip::tcp::socket socket_;
    std::shared_ptr<Room> room_;
    std::array<char, 1024> readBuffer_;
    std::deque<std::string> writeQueue_;
    int playerId_;
};
