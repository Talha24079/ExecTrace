#include "../include/exectrace.hpp"
#include "httplib.h"   // <--- FIXED: Changed from "../../include/httplib.h"
#include <iostream>

// Platform-specific headers for RAM usage
#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
    #pragma comment(lib, "psapi.lib") // Auto-link for Visual Studio
#else
    #include <sys/resource.h>
#endif

namespace
{
    std::string g_api_key;
    std::string g_app_version;
    std::string g_host;
    int g_port;
    bool g_initialized = false;
}

namespace ExecTrace
{
    void init(const std::string& api_key, const std::string& app_version, const std::string& host, int port)
    {
        g_api_key = api_key;
        g_app_version = app_version;
        g_host = host;
        g_port = port;
        g_initialized = true;
    }

    void log(const std::string& func_name, uint64_t duration_ms, uint64_t ram_usage_kb, const std::string& msg)
    {
        if (!g_initialized)
        {
            std::cerr << "[ExecTrace] Error: Client not initialized. Call ExecTrace::init() first." << std::endl;
            return;
        }

        // Create client
        httplib::Client cli(g_host, g_port);
        cli.set_connection_timeout(1, 0);

        // Prepare parameters
        httplib::Params params;
        params.emplace("func", func_name);
        params.emplace("duration", std::to_string(duration_ms));
        params.emplace("ram", std::to_string(ram_usage_kb));
        params.emplace("rom", "0");
        params.emplace("msg", msg.empty() ? "SDK_Log" : msg);
        params.emplace("app_version", g_app_version);  // NEW: Send app version

        // Add headers
        httplib::Headers headers = { {"X-API-Key", g_api_key} };

        // Send Request
        auto res = cli.Post("/log", headers, params);

        if (!res || res->status != 200)
        {
            std::cerr << "[ExecTrace] Warning: Failed to send log. Status: "
                      << (res ? std::to_string(res->status) : "Connection Failed") << std::endl;
        }
    }

    // FIXED: Cross-platform RAM check
    uint64_t get_current_ram_kb()
    {
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
        {
            return pmc.PrivateUsage / 1024; // Convert Bytes to KB
        }
        return 0;
#else
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0)
        {
            return usage.ru_maxrss; // Linux returns KB (usually)
        }
        return 0;
#endif
    }

    ScopedTrace::ScopedTrace(const std::string& func_name)
        : name(func_name)
    {
        start_time = std::chrono::high_resolution_clock::now();
    }

    ScopedTrace::~ScopedTrace()
    {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

        uint64_t ram = get_current_ram_kb();

        log(name, duration, ram, "Auto-Trace");
    }
}