#define WIN32_LEAN_AND_MEAN
#include "exectrace.hpp"
#include "../backend/include/httplib.h"

#include <iostream>
#include <mutex>
#include <memory>

#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
    #pragma comment(lib, "psapi.lib")
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/resource.h>
#endif

namespace {
    std::string g_project_key;
    std::string g_app_version;
    std::unique_ptr<httplib::Client> g_client;
    std::mutex g_client_mutex;
    bool g_initialized = false;
}

namespace ExecTrace {

void init(const std::string& project_api_key, const std::string& app_version, const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(g_client_mutex);

    g_project_key = project_api_key;
    g_app_version = app_version;
    
    g_client = std::make_unique<httplib::Client>(host, port);
    g_client->set_keep_alive(true);
    g_client->set_connection_timeout(2, 0);
    
    g_initialized = true;
    std::cout << "[ExecTrace] Initialized" << std::endl;
}

void log(const std::string& func_name, uint64_t duration_ms, uint64_t ram_usage_kb, const std::string& msg) {
    if (!g_initialized) return;

    httplib::Params params;
    params.emplace("func", func_name);
    params.emplace("duration", std::to_string(duration_ms));
    params.emplace("ram", std::to_string(ram_usage_kb));
    params.emplace("msg", msg.empty() ? "Trace" : msg);
    params.emplace("app_version", g_app_version);

    httplib::Headers headers = {
        {"X-API-Key", g_project_key},
        {"Connection", "Keep-Alive"}
    };

    std::lock_guard<std::mutex> lock(g_client_mutex);
    if (g_client) {
        g_client->Post("/log", headers, params);
    }
}

uint64_t get_current_ram_kb() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return pmc.PrivateUsage / 1024;
    }
    return 0;
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return usage.ru_maxrss;
    }
    return 0;
#endif
}

ScopedTrace::ScopedTrace(const std::string& func_name) : name(func_name) {
    start_time = std::chrono::high_resolution_clock::now();
}

ScopedTrace::~ScopedTrace() {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    uint64_t ram = get_current_ram_kb();
    
    log(name, duration, ram, "Auto");
}

}
