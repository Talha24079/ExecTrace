#pragma once
#include <map>
#include <string>
#include <chrono>
#include <mutex>

class RateLimiter {
private:
    struct TokenBucket {
        double tokens;
        std::chrono::steady_clock::time_point last_refill;
        double capacity;
        double refill_rate; // tokens per second
        
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
        // Global: 1000 requests per minute
    }
    
    // Check if request is allowed for a specific API key
    bool allow_request(const std::string& api_key) {
        std::lock_guard<std::mutex> lock(limiter_mutex);
        
        // Check global limit first
        if (!global_bucket.consume()) {
            return false;
        }
        
        // Check per-API-key limit
        if (api_key.empty()) {
            // Anonymous requests share a bucket
            if (api_key_buckets.find("__anonymous__") == api_key_buckets.end()) {
                api_key_buckets["__anonymous__"] = TokenBucket(20, 20.0 / 60.0); // 20/min for anonymous
            }
            return api_key_buckets["__anonymous__"].consume();
        }
        
        // Create bucket if not exists
        if (api_key_buckets.find(api_key) == api_key_buckets.end()) {
            api_key_buckets[api_key] = TokenBucket(100, 100.0 / 60.0); // 100/min per API key
        }
        
        return api_key_buckets[api_key].consume();
    }
    
    // Get remaining tokens for an API key
    double get_remaining(const std::string& api_key) {
        std::lock_guard<std::mutex> lock(limiter_mutex);
        
        if (api_key_buckets.find(api_key) == api_key_buckets.end()) {
            return 100.0; // Full bucket for new key
        }
        
        api_key_buckets[api_key].refill();
        return api_key_buckets[api_key].tokens;
    }
    
    // Cleanup old buckets periodically
    void cleanup() {
        std::lock_guard<std::mutex> lock(limiter_mutex);
        
        auto now = std::chrono::steady_clock::now();
        auto it = api_key_buckets.begin();
        while (it != api_key_buckets.end()) {
            auto elapsed = std::chrono::duration<double>(now - it->second.last_refill).count();
            // Remove buckets inactive for > 1 hour
            if (elapsed > 3600) {
                it = api_key_buckets.erase(it);
            } else {
                ++it;
            }
        }
    }
};
