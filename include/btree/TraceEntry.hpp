#pragma once
#include <string>
#include <cstring>
#include <iostream>

using namespace std;

struct TraceEntry
{
    int id;
    int user_id;   
    int version;   
    char timestamp[16];
    char process_name[32];
    char message[64];
    uint64_t duration;
    uint64_t ram_usage;
    uint64_t rom_usage;

    TraceEntry()
    {
        id = 0;
        user_id = 0;
        version = 0;
        memset(timestamp, 0, sizeof(timestamp));
        memset(process_name, 0, sizeof(process_name));
        memset(message, 0, sizeof(message));
        duration = 0;
        ram_usage = 0;
        rom_usage = 0;
    }

    void set(int _id, int _uid, int _ver, const char* _time, const char* _proc, const char* _msg, uint64_t _dur, uint64_t _ram, uint64_t _rom)
    {
        id = _id;
        user_id = _uid;
        version = _ver;
        strncpy(timestamp, _time, 15);
        strncpy(process_name, _proc, 31);
        strncpy(message, _msg, 63);
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
        os << "[ID:" << entry.id << "] User:" << entry.user_id << " (v" << entry.version << ") " << entry.process_name;
        return os;
    }
    
    void print() const {
        cout << "ID: " << id 
             << " | User: " << user_id 
             << " | Ver: " << version
             << " | Func: " << process_name 
             << " | RAM: " << ram_usage 
             << " | DUR: " << duration << endl;
    }
};