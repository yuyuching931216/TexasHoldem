#pragma once
#include <boost/asio.hpp>
#include <memory>

class Room;

// Server類別 - 管理網路連接和遊戲房間
class Server {
public:
    Server(boost::asio::io_context& ioContext, short port);
    
private:
    void doAccept();
    
    boost::asio::ip::tcp::acceptor acceptor_;
    std::shared_ptr<Room> room_;
};
