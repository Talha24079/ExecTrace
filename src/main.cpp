#include <iostream>
#include <string>
#include "btree/DiskManager.hpp"
#include "btree/BTree.hpp"

using namespace std;

int main()
{
    remove("real_trace.db");

    DiskManager dm("real_trace.db");
    BTree tree(&dm);

    cout << "--- Inserting Full Trace Data (RAM/ROM/Duration) ---" << endl;

    for (int i = 1; i <= 20; i++)
    {
        TraceEntry t;
        string msg = "function_call_" + to_string(i);
        
        // ID, Time, Process, Message, Duration, RAM, ROM
        t.set(i * 10, "12:00:00", "core_system", msg.c_str(), 
              i * 50,      // Duration (50, 100, 150...)
              i * 1024,    // RAM (1KB, 2KB...)
              50000);      // ROM (Fixed size)
        
        tree.insert(t);
    }

    cout << "--- Tree Structure ---" << endl;
    tree.print_tree();

    cout << "\n--- Verifying Data Integrity (ID 50) ---" << endl;
    vector<TraceEntry> results = tree.range_search(50, 50);
    
    if (!results.empty())
    {
        TraceEntry& res = results[0];
        cout << "ID: " << res.id << endl;
        cout << "Process: " << res.process_name << endl;
        cout << "Duration: " << res.duration << " micros" << endl;
        cout << "RAM Usage: " << res.ram_usage << " bytes" << endl;
    }
    else
    {
        cout << "Error: ID 50 not found." << endl;
    }

    return 0;
}