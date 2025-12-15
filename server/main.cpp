#include "Server.h"
#include "Logger.h"
#include <boost/asio.hpp>
#include <iostream>
#include <exception>
#include <cstring>
#include <csignal>

// 全局 io_context 指針，用於信號處理
boost::asio::io_context* g_ioContext = nullptr;

void signalHandler(int signal) {
    LOG_INFO("Main", "Received signal " + std::to_string(signal) + ", shutting down...");
    if (g_ioContext) {
        g_ioContext->stop();
    }
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [port] [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --no-auto-start    Disable auto-start (require START command)" << std::endl;
    std::cout << "  --auto-start       Enable auto-start (default)" << std::endl;
    std::cout << "  --no-console       Disable console output (log to file only)" << std::endl;
    std::cout << "  --log-dir <dir>    Set log directory (default: log)" << std::endl;
    std::cout << "  --help, -h         Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << " 8888                    # Start on port 8888 with auto-start" << std::endl;
    std::cout << "  " << programName << " 8888 --no-auto-start    # Require START command" << std::endl;
    std::cout << "  " << programName << " 8888 --log-dir logs     # Use 'logs' directory" << std::endl;
}

int main(int argc, char* argv[]) {
    try {
        // 預設設定
        short port = 8888;
        bool autoStart = true;
        bool consoleOutput = true;
        std::string logDir = "log";
        
        // 解析命令行參數
        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
                printUsage(argv[0]);
                return 0;
            }
            else if (std::strcmp(argv[i], "--no-auto-start") == 0) {
                autoStart = false;
            }
            else if (std::strcmp(argv[i], "--auto-start") == 0) {
                autoStart = true;
            }
            else if (std::strcmp(argv[i], "--no-console") == 0) {
                consoleOutput = false;
            }
            else if (std::strcmp(argv[i], "--log-dir") == 0 && i + 1 < argc) {
                logDir = argv[++i];
            }
            else if (argv[i][0] != '-') {
                // 假設是端口號
                port = static_cast<short>(std::atoi(argv[i]));
            }
        }
        
        // 初始化日誌系統
        Logger::getInstance().setConsoleOutput(consoleOutput);
        if (!Logger::getInstance().initialize(logDir)) {
            std::cerr << "Failed to initialize logger!" << std::endl;
            return 1;
        }
        
        LOG_INFO("Main", "=== Texas Hold'em Poker Server ===");
        LOG_INFO("Main", "Starting server on port " + std::to_string(port) + "...");
        LOG_INFO("Main", "Auto-start: " + std::string(autoStart ? "enabled" : "disabled"));
        LOG_INFO("Main", "Log file: " + Logger::getInstance().getCurrentLogFile());
        
        // 設置信號處理
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
        
        boost::asio::io_context ioContext;
        g_ioContext = &ioContext;
        
        Server server(ioContext, port, autoStart);
        
        LOG_INFO("Main", "Server is running. Press Ctrl+C to stop.");
        ioContext.run();
        
        LOG_INFO("Main", "Server stopped.");
        Logger::getInstance().shutdown();
        
    } catch (const std::exception& e) {
        LOG_ERROR("Main", std::string("Exception: ") + e.what());
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
