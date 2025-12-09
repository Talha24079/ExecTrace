#include <iostream>
#include <string>
#include "btree/ExecTraceDB.hpp"

using namespace std;

int main()
{
    // Clean up old files
    remove("test_sys_main.db");
    remove("test_sys_ram.db");
    remove("test_sys_dur.db");

    cout << "--- Initializing Multi-Index DB ---" << endl;
    ExecTraceDB db("test_sys");

    cout << "--- Inserting Events (Auto-Generating IDs) ---" << endl;
    
    // REMOVED the first integer argument (ID).
    // New Format: Func, Msg, Duration, RAM, ROM
    int id1 = db.log_event("func_fast", "ok",     10,   100, 0);
    int id2 = db.log_event("func_slow", "heavy",  5000, 200, 0); 
    int id3 = db.log_event("func_mid",  "normal", 150,  5000, 0); 
    int id4 = db.log_event("func_lag",  "laggy",  5000, 100, 0); 

    cout << "Generated IDs: " << id1 << ", " << id2 << ", " << id3 << ", " << id4 << endl;

    cout << "\n--- Query 1: Find Slow Functions (> 1000ms) ---" << endl;
    vector<TraceEntry> slow_logs = db.query_by_duration(1000, 6000);
    
    for(const auto& log : slow_logs)
    {
        cout << "FOUND: " << log.process_name << " (Time: " << log.duration << "ms)" << endl;
    }

    cout << "\n--- Query 2: Find High RAM Usage (> 1000 bytes) ---" << endl;
    vector<TraceEntry> fat_logs = db.query_by_ram(1000, 10000);
    
    for(const auto& log : fat_logs)
    {
        cout << "FOUND: " << log.process_name << " (RAM: " << log.ram_usage << " bytes)" << endl;
    }

    return 0;
}