#include "Server.h"
#include <boost/asio.hpp>
#include <iostream>
#include <exception>

int main(int argc, char* argv[]) {
    try {
        // 預設端口為8888
        short port = 8888;
        
        if (argc > 1) {
            port = static_cast<short>(std::atoi(argv[1]));
        }
        
        std::cout << "=== Texas Hold'em Poker Server ===" << std::endl;
        std::cout << "Starting server on port " << port << "..." << std::endl;
        
        boost::asio::io_context ioContext;
        Server server(ioContext, port);
        
        std::cout << "Server is running. Press Ctrl+C to stop." << std::endl;
        ioContext.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
