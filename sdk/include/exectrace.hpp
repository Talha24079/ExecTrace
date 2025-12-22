#pragma once

#include <string>
#include <cstdint>
#include <chrono>

namespace ExecTrace
{
    void init(const std::string& api_key, const std::string& app_version = "v1.0.0", const std::string& host = "127.0.0.1", int port = 8080);
    void log(const std::string& func_name, uint64_t duration_ms, uint64_t ram_usage_kb, const std::string& msg = "");

    class ScopedTrace
    {
    private:
        std::string name;
        std::chrono::time_point<std::chrono::high_resolution_clock> start_time;

    public:
        ScopedTrace(const std::string& func_name);
        ~ScopedTrace();
    };
}

#define TRACE_FUNCTION() ExecTrace::ScopedTrace _et_trace(__FUNCTION__)
#define TRACE_SCOPE(name) ExecTrace::ScopedTrace _et_trace(name)