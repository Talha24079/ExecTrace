#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "DiskManager.hpp"
#include "BTree.hpp"
#include "TraceEntry.hpp"

using namespace std;

class ExecTraceDB
{
private:
    string db_name;
    DiskManager* dm;
    BTree* main_tree;

public:
    ExecTraceDB(string filename)
    {
        db_name = filename;
        dm = new DiskManager(db_name);
        main_tree = new BTree(dm);
    }

    ~ExecTraceDB()
    {
        delete main_tree;
        delete dm;
    }

    void log_event(int id, const char* func, const char* msg, uint64_t duration, uint64_t ram, uint64_t rom)
    {
        TraceEntry entry;
        entry.set(id, "12:00:00", func, msg, duration, ram, rom);
        main_tree->insert(entry);
    }

    TraceEntry get_event(int id)
    {
        vector<TraceEntry> res = main_tree->range_search(id, id);
        if (!res.empty()) 
            return res[0];
        return TraceEntry();
    }

    vector<TraceEntry> query_range(int start_id, int end_id)
    {
        return main_tree->range_search(start_id, end_id);
    }

    void debug_dump()
    {
        main_tree->print_tree();
    }
};