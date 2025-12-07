#include <iostream>
#include <string>
#include "btree/ExecTraceDB.hpp"

using namespace std;

int main()
{
    // Clean up old files so we start fresh
    remove("test_sys_main.db");
    remove("test_sys_ram.db");
    remove("test_sys_dur.db");

    cout << "--- Initializing Multi-Index DB ---" << endl;
    ExecTraceDB db("test_sys");

    cout << "--- Inserting Events ---" << endl;
    // ID, Func, Msg, Duration, RAM, ROM
    db.log_event(1, "func_fast", "ok",     10,   100, 0);
    db.log_event(2, "func_slow", "heavy",  5000, 200, 0); // Slow!
    db.log_event(3, "func_mid",  "normal", 150,  5000, 0); // High RAM!
    db.log_event(4, "func_lag",  "laggy",  5000, 100, 0); // Slow!

    cout << "\n--- Query 1: Find Slow Functions (> 1000ms) ---" << endl;
    // This should find ID 2 and ID 4, even though their IDs are far apart.
    // It works because the Duration Index sorts them together.
    vector<TraceEntry> slow_logs = db.query_by_duration(1000, 6000);
    
    for(const auto& log : slow_logs)
    {
        cout << "FOUND: " << log.process_name << " (Time: " << log.duration << "ms)" << endl;
    }

    cout << "\n--- Query 2: Find High RAM Usage (> 1000 bytes) ---" << endl;
    // Should find ID 3
    vector<TraceEntry> fat_logs = db.query_by_ram(1000, 10000);
    
    for(const auto& log : fat_logs)
    {
        cout << "FOUND: " << log.process_name << " (RAM: " << log.ram_usage << " bytes)" << endl;
    }

    return 0;
}