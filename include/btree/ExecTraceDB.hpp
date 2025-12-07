#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "DiskManager.hpp"
#include "BTree.hpp"
#include "TraceEntry.hpp"
#include "Hasher.hpp" // <--- NEW

using namespace std;

class ExecTraceDB
{
private:
    string db_base_name;
    DiskManager* main_dm;
    BTree* main_tree;

    DiskManager* ram_dm;
    BTree* ram_tree;

    DiskManager* dur_dm;
    BTree* dur_tree;

public:
    ExecTraceDB(string filename)
    {
        db_base_name = filename;
        
        main_dm = new DiskManager(filename + "_main.db");
        main_tree = new BTree(main_dm);

        ram_dm = new DiskManager(filename + "_ram.db");
        ram_tree = new BTree(ram_dm);

        dur_dm = new DiskManager(filename + "_dur.db");
        dur_tree = new BTree(dur_dm);
    }

    ~ExecTraceDB()
    {
        delete main_tree; delete main_dm;
        delete ram_tree;  delete ram_dm;
        delete dur_tree;  delete dur_dm;
    }

    int log_event(const char* func, const char* msg, uint64_t duration, uint64_t ram, uint64_t rom)
    {
        static uint64_t counter = 1000; 
        counter++;
        
        uint64_t generated_id = Hasher::generate_unique_id(func, counter);
        
        int id = (int)(generated_id % 1000000); 

        TraceEntry main_entry;
        main_entry.set(id, "12:00:00", func, msg, duration, ram, rom);
        main_tree->insert(main_entry);

        TraceEntry ram_entry;
        ram_entry.set((int)ram, "IDX", "RAM_IDX", "PTR", 0, (uint64_t)id, 0);
        ram_tree->insert(ram_entry);

        TraceEntry dur_entry;
        dur_entry.set((int)duration, "IDX", "DUR_IDX", "PTR", (uint64_t)id, 0, 0);
        dur_tree->insert(dur_entry);

        return id; 
    }

    vector<TraceEntry> query_range(int start_id, int end_id)
    {
        return main_tree->range_search(start_id, end_id);
    }
    
    vector<TraceEntry> query_by_ram(int min_ram, int max_ram) 
    {
        vector<TraceEntry> results;
        vector<TraceEntry> index_hits = ram_tree->range_search(min_ram, max_ram);

        for(const auto& idx : index_hits) 
        {
            int real_id = (int)idx.ram_usage; 
            vector<TraceEntry> full_record = main_tree->range_search(real_id, real_id);
            if (!full_record.empty()) results.push_back(full_record[0]);
        }
        return results;
    }
    
    vector<TraceEntry> query_by_duration(int min_dur, int max_dur) 
    {
        vector<TraceEntry> results;
        vector<TraceEntry> index_hits = dur_tree->range_search(min_dur, max_dur);

        for(const auto& idx : index_hits) 
        {
            int real_id = (int)idx.duration;
            vector<TraceEntry> full_record = main_tree->range_search(real_id, real_id);
            if (!full_record.empty()) results.push_back(full_record[0]);
        }
        return results;
    }

    void debug_dump()
    {
        main_tree->print_tree();
    }
};