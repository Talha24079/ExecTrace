#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>

class Logger {
public:
    enum Level {
        DEBUG = 0,
        INFO = 1,
        WARN = 2,
        ERROR = 3
    };

private:
    std::ofstream log_file;
    Level min_level;
    std::mutex log_mutex;
    
    std::string get_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    
    std::string level_to_string(Level level) {
        switch (level) {
            case DEBUG: return "DEBUG";
            case INFO:  return "INFO ";
            case WARN:  return "WARN ";
            case ERROR: return "ERROR";
            default:    return "?????";
        }
    }

public:
    Logger(const std::string& filename = "backend/data/server.log", Level min_level = INFO) 
        : min_level(min_level) {
        log_file.open(filename, std::ios::app);
        if (!log_file.is_open()) {
            std::cerr << "[Logger] Failed to open log file: " << filename << std::endl;
        } else {
            log(INFO, "Logger", "Logger initialized");
        }
    }
    
    ~Logger() {
        if (log_file.is_open()) {
            log(INFO, "Logger", "Logger shutting down");
            log_file.close();
        }
    }
    
    void log(Level level, const std::string& component, const std::string& message) {
        if (level < min_level) return;
        
        std::lock_guard<std::mutex> lock(log_mutex);
        
        std::string log_entry = "[" + get_timestamp() + "] [" + level_to_string(level) + 
                               "] [" + component + "] " + message;
        
        // Write to file
        if (log_file.is_open()) {
            log_file << log_entry << std::endl;
            log_file.flush();
        }
        
        // Also write to console for errors
        if (level >= WARN) {
            std::cerr << log_entry << std::endl;
        }
    }
    
    void debug(const std::string& component, const std::string& message) {
        log(DEBUG, component, message);
    }
    
    void info(const std::string& component, const std::string& message) {
        log(INFO, component, message);
    }
    
    void warn(const std::string& component, const std::string& message) {
        log(WARN, component, message);
    }
    
    void error(const std::string& component, const std::string& message) {
        log(ERROR, component, message);
    }
};

// Global logger instance
static Logger* g_logger = nullptr;

inline void init_logger(const std::string& filename = "backend/data/server.log") {
    if (!g_logger) {
        g_logger = new Logger(filename);
    }
}

inline void log_info(const std::string& component, const std::string& message) {
    if (g_logger) g_logger->info(component, message);
}

inline void log_warn(const std::string& component, const std::string& message) {
    if (g_logger) g_logger->warn(component, message);
}

inline void log_error(const std::string& component, const std::string& message) {
    if (g_logger) g_logger->error(component, message);
}

inline void log_debug(const std::string& component, const std::string& message) {
    if (g_logger) g_logger->debug(component, message);
}
