#pragma once
#define _CRT_SECURE_NO_WARNINGS 
#include <string>
#include <chrono>
#include <thread>
#include <cstring>
#include <iostream>
#include <cmath>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib") 
#elif __linux__
#include <unistd.h>
#include <fstream>
#endif

namespace ExecTrace {

    static std::string g_api_key;
    static std::string g_app_version;
    static std::string g_server_url;

    inline long get_current_ram_kb() {
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS_EX pmc;
        GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
        return pmc.WorkingSetSize / 1024;
#elif __linux__
        std::ifstream status("/proc/self/status");
        std::string line;
        while (std::getline(status, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                return std::stol(line.substr(7));
            }
        }
        return 0;
#else
        return 0;
#endif
    }

    inline std::string auto_version() {
        const char* env_ver = std::getenv("APP_VERSION");
        if (env_ver) return std::string(env_ver);
        return "v1.0.0";  
    }

    inline void init(const std::string& api_key, const std::string& version = "",
        const std::string& host = "127.0.0.1", int port = 9090) {
        g_api_key = api_key;
        g_app_version = version.empty() ? auto_version() : version;
        g_server_url = "http://" + host + ":" + std::to_string(port);

        std::cout << "[ExecTrace] Initialized. Project: " << api_key.substr(0, 8) << "..." << std::endl;
    }

    inline void log(const std::string& func_name, long duration_ms, long ram_kb,
        const std::string& message = "Manual trace") {
        std::string post_data = "func=" + func_name +
            "&message=" + message +
            "&duration=" + std::to_string(duration_ms) +
            "&ram=" + std::to_string(ram_kb) +
            "&version=" + g_app_version;

        std::thread([post_data]() {
#ifdef _WIN32
            
            std::string cmd = "curl -s -X POST " + g_server_url + "/log "
                "-H \"X-API-Key: " + g_api_key + "\" "
                "-d \"" + post_data + "\" > NUL 2>&1";
#else
            
            std::string cmd = "curl -s -X POST " + g_server_url + "/log "
                "-H 'X-API-Key: " + g_api_key + "' "
                "-d '" + post_data + "' > /dev/null 2>&1";
#endif
            int res = system(cmd.c_str());
            if (res != 0) std::cerr << "[ExecTrace] Warning: Curl failed with code " << res << std::endl;
            }).detach();
    }

    class ScopeTracer {
    private:
        std::string scope_name;
        std::chrono::high_resolution_clock::time_point start_time;

    public:
        ScopeTracer(const std::string& name) : scope_name(name) {
            start_time = std::chrono::high_resolution_clock::now();
        }

        ~ScopeTracer() {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time - start_time).count();
            log(scope_name, duration, get_current_ram_kb(), "Auto-trace");
        }
    };

}

#define TRACE_FUNCTION() ExecTrace::ScopeTracer __tracer__(__FUNCTION__)
#define TRACE_SCOPE(name) ExecTrace::ScopeTracer __tracer__(name)