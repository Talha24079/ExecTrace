#pragma once
#include <cstring>
#include <iostream>
#include <cstdint>

struct TraceEntry 
{
    int id;
    char timestamp[20];
    char process_name[32];
    char message[64];
    uint64_t duration;
    uint64_t ram_usage;
    uint64_t rom_size;

    TraceEntry() 
    {
        id = 0;
        std::memset(timestamp, 0, 20);
        std::memset(process_name, 0, 32);
        std::memset(message, 0, 64);
        duration = 0;
        ram_usage = 0;
        rom_size = 0;
    }

    void set(int k, const char* t, const char* p, const char* m, uint64_t dur, uint64_t ram, uint64_t rom) 
    {
        id = k;
        strncpy(timestamp, t, 20);
        strncpy(process_name, p, 32);
        strncpy(message, m, 64);
        duration = dur;
        ram_usage = ram;
        rom_size = rom;
    }
};