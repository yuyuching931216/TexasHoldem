#include "Server.h"
#include "Session.h"
#include "Room.h"
#include <iostream>

Server::Server(boost::asio::io_context& ioContext, short port, bool autoStart)
    : acceptor_(ioContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      room_(std::make_shared<Room>(1, autoStart)) {
    
    std::cout << "Texas Hold'em Server started on port " << port << std::endl;
    std::cout << "Auto-start: " << (autoStart ? "enabled" : "disabled") << std::endl;
    std::cout << "Waiting for players..." << std::endl;
    
    doAccept();
}

void Server::doAccept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                std::cout << "New connection from " 
                         << socket.remote_endpoint().address().to_string() 
                         << ":" << socket.remote_endpoint().port() << std::endl;
                
                std::make_shared<Session>(std::move(socket), room_)->start();
            } else {
                std::cerr << "Accept error: " << ec.message() << std::endl;
            }
            
            doAccept();
        });
}
