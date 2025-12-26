#pragma once
#include <string>
#include <algorithm>
#include <cctype>
#include <regex>

namespace ExecTrace {

// Sanitize string to prevent XSS and injection attacks
inline std::string sanitize_string(const std::string& input, size_t max_length = 256) {
    std::string sanitized = input.substr(0, max_length);
    
    // Remove or escape dangerous characters
    std::string result;
    result.reserve(sanitized.length());
    
    for (char c : sanitized) {
        // Allow alphanumeric, space, and safe punctuation
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
        // Skip other potentially dangerous characters
    }
    
    return result;
}

// Validate API key format (sk_live_<hex>)
inline bool validate_api_key(const std::string& api_key) {
    if (api_key.length() < 10 || api_key.length() > 128) {
        return false;
    }
    
    // Check if it starts with expected prefix
    if (api_key.substr(0, 8) == "sk_live_") {
        // Rest should be hexadecimal
        std::string hex_part = api_key.substr(8);
        return std::all_of(hex_part.begin(), hex_part.end(), ::isxdigit);
    }
    
    // Allow any format for backward compatibility, but check for dangerous chars
    return std::all_of(api_key.begin(), api_key.end(), [](char c) {
        return std::isalnum(c) || c == '_' || c == '-';
    });
}

// Validate numeric bounds
inline bool validate_duration(uint64_t duration) {
    // Duration should be reasonable (0 to 1 hour in milliseconds)
    return duration <= 3600000;
}

inline bool validate_ram(uint64_t ram_kb) {
    // RAM should be reasonable (0 to 100GB in KB)
    return ram_kb <= 104857600;
}

// Validate project/user IDs
inline bool validate_id(int id) {
    return id > 0 && id < 1000000; // Reasonable bounds
}

// Validate email format (basic)
inline bool validate_email(const std::string& email) {
    if (email.length() < 3 || email.length() > 254) {
        return false;
    }
    
    // Basic email regex
    std::regex email_pattern(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    return std::regex_match(email, email_pattern);
}

// Validate username
inline bool validate_username(const std::string& username) {
    if (username.length() < 3 || username.length() > 32) {
        return false;
    }
    
    // Allow alphanumeric and underscore only
    return std::all_of(username.begin(), username.end(), [](char c) {
        return std::isalnum(c) || c == '_' || c == '-';
    });
}

// Validate pagination parameters
inline bool validate_pagination(int offset, int limit) {
    return offset >= 0 && limit > 0 && limit <= 1000;
}

} // namespace ExecTrace
