#pragma once
#include <string>
#include <algorithm>
#include <cctype>
#include <regex>
#include <map>
#include <chrono>
#include <mutex>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace ExecTrace {

inline std::string sanitize_string(const std::string& input, size_t max_length = 256) {
    std::string sanitized = input.substr(0, max_length);

    std::string result;
    result.reserve(sanitized.length());
    
    for (char c : sanitized) {
        
        if (std::isalnum(c) || c == ' ' || c == '_' || c == '-' || 
            c == '.' || c == '(' || c == ')' || c == ':' || c == '/') {
            result += c;
        } else if (c == '<') {
            result += "&lt;";
        } else if (c == '>') {
            result += "&gt;";
        } else if (c == '"') {
            result += "&quot;";
        } else if (c == '\'') {
            result += "&#39;";
        } else if (c == '&') {
            result += "&amp;";
        }
    }
    
    return result;
}

inline bool validate_api_key(const std::string& api_key) {
    if (api_key.length() < 10 || api_key.length() > 128) {
        return false;
    }

    if (api_key.substr(0, 8) == "sk_live_") {
        
        std::string hex_part = api_key.substr(8);
        return std::all_of(hex_part.begin(), hex_part.end(), ::isxdigit);
    }

    return std::all_of(api_key.begin(), api_key.end(), [](char c) {
        return std::isalnum(c) || c == '_' || c == '-';
    });
}

inline bool validate_duration(uint64_t duration) {
    
    return duration <= 3600000;
}

inline bool validate_ram(uint64_t ram_kb) {
    
    return ram_kb <= 104857600;
}

inline bool validate_id(int id) {
    return id > 0 && id < 1000000; 
}

inline bool validate_email(const std::string& email) {
    if (email.length() < 3 || email.length() > 254) {
        return false;
    }

    std::regex email_pattern(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    return std::regex_match(email, email_pattern);
}

inline bool validate_username(const std::string& username) {
    if (username.length() < 3 || username.length() > 32) {
        return false;
    }

    return std::all_of(username.begin(), username.end(), [](char c) {
        return std::isalnum(c) || c == '_' || c == '-';
    });
}

inline bool validate_pagination(int offset, int limit) {
    return offset >= 0 && limit > 0 && limit <= 1000;
}

} 

class RateLimiter {
private:
    struct TokenBucket {
        double tokens;
        std::chrono::steady_clock::time_point last_refill;
        double capacity;
        double refill_rate; 
        
        TokenBucket(double cap = 100, double rate = 100.0 / 60.0) 
            : tokens(cap), last_refill(std::chrono::steady_clock::now()), 
              capacity(cap), refill_rate(rate) {}
        
        void refill() {
            auto now = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(now - last_refill).count();
            tokens = std::min(capacity, tokens + elapsed * refill_rate);
            last_refill = now;
        }
        
        bool consume(double count = 1.0) {
            refill();
            if (tokens >= count) {
                tokens -= count;
                return true;
            }
            return false;
        }
    };
    
    std::map<std::string, TokenBucket> api_key_buckets;
    TokenBucket global_bucket;
    std::mutex limiter_mutex;
    
public:
    RateLimiter() : global_bucket(1000, 1000.0 / 60.0) {
        
    }

    bool allow_request(const std::string& api_key) {
        std::lock_guard<std::mutex> lock(limiter_mutex);

        if (!global_bucket.consume()) {
            return false;
        }

        if (api_key.empty()) {
            
            if (api_key_buckets.find("__anonymous__") == api_key_buckets.end()) {
                api_key_buckets["__anonymous__"] = TokenBucket(20, 20.0 / 60.0); 
            }
            return api_key_buckets["__anonymous__"].consume();
        }

        if (api_key_buckets.find(api_key) == api_key_buckets.end()) {
            api_key_buckets[api_key] = TokenBucket(100, 100.0 / 60.0); 
        }
        
        return api_key_buckets[api_key].consume();
    }

    double get_remaining(const std::string& api_key) {
        std::lock_guard<std::mutex> lock(limiter_mutex);
        
        if (api_key_buckets.find(api_key) == api_key_buckets.end()) {
            return 100.0; 
        }
        
        api_key_buckets[api_key].refill();
        return api_key_buckets[api_key].tokens;
    }

    void cleanup() {
        std::lock_guard<std::mutex> lock(limiter_mutex);
        
        auto now = std::chrono::steady_clock::now();
        auto it = api_key_buckets.begin();
        while (it != api_key_buckets.end()) {
            auto elapsed = std::chrono::duration<double>(now - it->second.last_refill).count();
            
            if (elapsed > 3600) {
                it = api_key_buckets.erase(it);
            } else {
                ++it;
            }
        }
    }
};

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

        if (log_file.is_open()) {
            log_file << log_entry << std::endl;
            log_file.flush();
        }

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
