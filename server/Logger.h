#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <iostream>
#include <filesystem>

// 取消 Windows 的 ERROR 宏定義
#ifdef ERROR
#undef ERROR
#endif

enum class LogLevel {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
};

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    // 禁止拷貝和賦值
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // 初始化日誌系統
    bool initialize(const std::string& logDir = "log") {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        
        logDir_ = logDir;
        
        // 創建日誌目錄
        try {
            if (!std::filesystem::exists(logDir_)) {
                std::filesystem::create_directories(logDir_);
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to create log directory: " << e.what() << std::endl;
            return false;
        }
        
        // 生成日誌文件名（包含日期時間）
        std::string filename = generateLogFilename();
        std::string filepath = logDir_ + "/" + filename;
        
        // 打開日誌文件
        logFile_.open(filepath, std::ios::out | std::ios::app);
        if (!logFile_.is_open()) {
            std::cerr << "Failed to open log file: " << filepath << std::endl;
            return false;
        }
        
        initialized_ = true;
        currentLogFile_ = filepath;
        
        // 直接寫入，不遞歸呼叫 log()
        std::string timestamp = getCurrentTimestamp();
        std::string logLine = "[" + timestamp + "] [INFO ] [Logger] Log system initialized. Log file: " + filepath;
        if (consoleOutput_) {
            std::cout << logLine << std::endl;
        }
        logFile_ << logLine << std::endl;
        logFile_.flush();
        
        return true;
    }

    // 關閉日誌系統
    void shutdown() {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (logFile_.is_open()) {
            logFile_.close();
        }
        initialized_ = false;
    }

    // 寫入日誌
    void log(LogLevel level, const std::string& source, const std::string& message) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        
        std::string timestamp = getCurrentTimestamp();
        std::string levelStr = levelToString(level);
        
        std::stringstream ss;
        ss << "[" << timestamp << "] [" << levelStr << "] [" << source << "] " << message;
        std::string logLine = ss.str();
        
        // 輸出到控制台
        if (consoleOutput_) {
            std::cout << logLine << std::endl;
        }
        
        // 輸出到文件
        if (initialized_ && logFile_.is_open()) {
            logFile_ << logLine << std::endl;
            logFile_.flush();  // 確保立即寫入
        }
    }

    // 便捷方法
    void debug(const std::string& source, const std::string& message) {
        log(LogLevel::LOG_DEBUG, source, message);
    }

    void info(const std::string& source, const std::string& message) {
        log(LogLevel::LOG_INFO, source, message);
    }

    void warning(const std::string& source, const std::string& message) {
        log(LogLevel::LOG_WARNING, source, message);
    }

    void error(const std::string& source, const std::string& message) {
        log(LogLevel::LOG_ERROR, source, message);
    }

    // 設置是否同時輸出到控制台
    void setConsoleOutput(bool enabled) {
        consoleOutput_ = enabled;
    }

    // 獲取當前日誌文件路徑
    std::string getCurrentLogFile() const {
        return currentLogFile_;
    }

private:
    Logger() : initialized_(false), consoleOutput_(true) {}
    
    ~Logger() {
        shutdown();
    }

    std::string generateLogFilename() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf;
#ifdef _WIN32
        localtime_s(&tm_buf, &time);
#else
        localtime_r(&time, &tm_buf);
#endif
        
        std::stringstream ss;
        ss << "server_" 
           << std::put_time(&tm_buf, "%Y%m%d_%H%M%S")
           << ".log";
        return ss.str();
    }

    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::tm tm_buf;
#ifdef _WIN32
        localtime_s(&tm_buf, &time);
#else
        localtime_r(&time, &tm_buf);
#endif
        
        std::stringstream ss;
        ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S")
           << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    std::string levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::LOG_DEBUG:   return "DEBUG";
            case LogLevel::LOG_INFO:    return "INFO ";
            case LogLevel::LOG_WARNING: return "WARN ";
            case LogLevel::LOG_ERROR:   return "ERROR";
            default:                    return "?????";
        }
    }

    std::recursive_mutex mutex_;  // 使用遞歸互斥鎖
    std::ofstream logFile_;
    std::string logDir_;
    std::string currentLogFile_;
    bool initialized_;
    bool consoleOutput_;
};

// 便捷宏定義
#define LOG_DEBUG(source, msg)   Logger::getInstance().debug(source, msg)
#define LOG_INFO(source, msg)    Logger::getInstance().info(source, msg)
#define LOG_WARNING(source, msg) Logger::getInstance().warning(source, msg)
#define LOG_ERROR(source, msg)   Logger::getInstance().error(source, msg)
