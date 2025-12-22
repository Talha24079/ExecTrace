#pragma once
#include <string>
#include <cstring>
#include <iostream>
#include <cstdint> 

using namespace std;

struct TraceEntry
{
    int id;
    int project_id; 
    int func_call_count;  // Renamed from version - counts function call iterations
    char timestamp[16];
    char process_name[32];
    char message[64];
    char app_version[16];  // NEW - application version (e.g., "v1.0.0")
    uint64_t duration;
    uint64_t ram_usage;
    uint64_t rom_usage;

    TraceEntry()
    {
        id = 0;
        project_id = 0;
        func_call_count = 0;
        memset(timestamp, 0, sizeof(timestamp));
        memset(process_name, 0, sizeof(process_name));
        memset(message, 0, sizeof(message));
        memset(app_version, 0, sizeof(app_version));
        duration = 0;
        ram_usage = 0;
        rom_usage = 0;
    }

    void set(int _id, int _pid, int _func_cnt, const char* _time, const char* _proc, 
             const char* _msg, const char* _app_ver, uint64_t _dur, uint64_t _ram, uint64_t _rom)
    {
        id = _id;
        project_id = _pid;
        func_call_count = _func_cnt;
        strncpy(timestamp, _time, 15);
        timestamp[15] = '\0';
        strncpy(process_name, _proc, 31);
        process_name[31] = '\0';
        strncpy(message, _msg, 63);
        message[63] = '\0';
        strncpy(app_version, _app_ver, 15);
        app_version[15] = '\0';
        duration = _dur;
        ram_usage = _ram;
        rom_usage = _rom;
    }

    bool operator<(const TraceEntry& other) const { return id < other.id; }
    bool operator>(const TraceEntry& other) const { return id > other.id; }
    bool operator==(const TraceEntry& other) const { return id == other.id; }
    bool operator<=(const TraceEntry& other) const { return id <= other.id; }
    bool operator>=(const TraceEntry& other) const { return id >= other.id; }

    friend ostream& operator<<(ostream& os, const TraceEntry& entry)
    {
        os << "[ID:" << entry.id << "] Proj:" << entry.project_id << " (" << entry.app_version << ", call#" << entry.func_call_count << ") " << entry.process_name;
        return os;
    }

    void print() const
    {
        cout << "ID: " << id
             << " | Proj: " << project_id
             << " | Ver: " << app_version
             << " | Call#: " << func_call_count
             << " | Func: " << process_name
             << " | RAM: " << ram_usage
             << " | DUR: " << duration << endl;
    }
};