#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <mutex>
#include <unordered_map> 
#include "DiskManager.hpp"
#include "BTree.hpp"
#include "TraceEntry.hpp"
#include "Hasher.hpp"

using namespace std;

class ExecTraceDB
{
private:
    string db_base_name;
    mutex db_mutex; 

    // key: "projectID_funcName", value: version
    unordered_map<string, int> version_map; 

    DiskManager* main_dm;
    BTree<TraceEntry>* main_tree;

    DiskManager* ram_dm;
    BTree<TraceEntry>* ram_tree;

    DiskManager* dur_dm;
    BTree<TraceEntry>* dur_tree;

public:
    ExecTraceDB(string filename)
    {
        db_base_name = filename;

        main_dm = new DiskManager(filename + "_main.db");
        main_tree = new BTree<TraceEntry>(main_dm);

        ram_dm = new DiskManager(filename + "_ram.db");
        ram_tree = new BTree<TraceEntry>(ram_dm);

        dur_dm = new DiskManager(filename + "_dur.db");
        dur_tree = new BTree<TraceEntry>(dur_dm);
    }

    ~ExecTraceDB()
    {
        delete main_tree; delete main_dm;
        delete ram_tree;  delete ram_dm;
        delete dur_tree;  delete dur_dm;
    }

    // CHANGED: user_id -> project_id, added app_version parameter
    int log_event(int project_id, const char* func, const char* msg, const char* app_version, 
                  uint64_t duration, uint64_t ram, uint64_t rom)
    {
        lock_guard<mutex> lock(db_mutex);

        // Versioning is now per-project
        string version_key = to_string(project_id) + "_" + string(func);
        int current_version = 1;

        if (version_map.find(version_key) != version_map.end())
        {
            current_version = version_map[version_key] + 1;
        }
        version_map[version_key] = current_version;

        static uint64_t counter = 1000;
        counter++;

        uint64_t generated_id = Hasher::generate_unique_id(func, counter);
        int id = (int)(generated_id & 0x7FFFFFFF);

        TraceEntry main_entry;
        // CHANGED: passing project_id and app_version to set()
        main_entry.set(id, project_id, current_version, "12:00:00", func, msg, app_version, duration, ram, rom);
        main_tree->insert(main_entry);

        // Secondary Indexes (RAM and Duration)
        // Note: We don't store project_id in the index key itself (to save space), 
        // we look it up in the main record during query.
        
        TraceEntry ram_entry;
        ram_entry.set((int)ram, 0, 0, "IDX", "RAM_IDX", "PTR", "", 0, (uint64_t)id, 0);
        ram_tree->insert(ram_entry);

        TraceEntry dur_entry;
        dur_entry.set((int)duration, 0, 0, "IDX", "DUR_IDX", "PTR", "", (uint64_t)id, 0, 0);
        dur_tree->insert(dur_entry);

        return id;
    }

    // CHANGED: user_id -> project_id
    vector<TraceEntry> query_range(int project_id, int start_id, int end_id)
    {
        lock_guard<mutex> lock(db_mutex); 
        TraceEntry start_key; start_key.id = start_id;
        TraceEntry end_key; end_key.id = end_id;
        
        vector<TraceEntry> all = main_tree->range_search(start_key, end_key);
        
        vector<TraceEntry> filtered;
        for(const auto& entry : all) {
            // CHANGED: Filtering by project_id member
            if (entry.project_id == project_id) {
                filtered.push_back(entry);
            }
        }
        return filtered;
    }

    // CHANGED: user_id -> project_id
    vector<TraceEntry> query_by_ram(int project_id, int min_ram, int max_ram)
    {
        lock_guard<mutex> lock(db_mutex); 
        vector<TraceEntry> results;
        
        TraceEntry start_key; start_key.id = min_ram;
        TraceEntry end_key; end_key.id = max_ram;
        
        // 1. Search RAM Index
        vector<TraceEntry> index_hits = ram_tree->range_search(start_key, end_key);

        // 2. Lookup Main Record
        for (const auto& idx : index_hits) {
            int real_id = (int)idx.ram_usage; // We stored the Real ID in ram_usage field of index
            
            TraceEntry target_key; target_key.id = real_id;
            vector<TraceEntry> full_record = main_tree->range_search(target_key, target_key);
            
            // 3. Verify Project ID
            if (!full_record.empty() && full_record[0].project_id == project_id) {
                results.push_back(full_record[0]);
            }
        }
        return results;
    }

    // CHANGED: user_id -> project_id
    vector<TraceEntry> query_by_duration(int project_id, int min_dur, int max_dur)
    {
        lock_guard<mutex> lock(db_mutex); 
        vector<TraceEntry> results;
        
        TraceEntry start_key; start_key.id = min_dur;
        TraceEntry end_key; end_key.id = max_dur;
        
        vector<TraceEntry> index_hits = dur_tree->range_search(start_key, end_key);

        for (const auto& idx : index_hits) {
            int real_id = (int)idx.duration; // Stored Real ID in duration field of index
            
            TraceEntry target_key; target_key.id = real_id;
            vector<TraceEntry> full_record = main_tree->range_search(target_key, target_key);
            
            if (!full_record.empty() && full_record[0].project_id == project_id) {
                results.push_back(full_record[0]);
            }
        }
        return results;
    }

    // Smart Query Engine: Server-side sorting, filtering, and pagination
    vector<TraceEntry> smart_query(int project_id, string sort_by, string order, int limit, int offset)
    {
        lock_guard<mutex> lock(db_mutex);
        vector<TraceEntry> results;
        
        if (sort_by == "duration") {
            // Use duration B-Tree index for efficient sorting
            TraceEntry min_key, max_key;
            min_key.id = 0; 
            max_key.id = INT_MAX;
            vector<TraceEntry> idx = dur_tree->range_search(min_key, max_key);
            
            // Fetch full records from main tree
            for (const auto& entry : idx) {
                int real_id = (int)entry.duration; // Real ID stored in duration field
                TraceEntry target; 
                target.id = real_id;
                auto rec = main_tree->range_search(target, target);
                if (!rec.empty() && rec[0].project_id == project_id) {
                    results.push_back(rec[0]);
                }
            }
        } else if (sort_by == "ram") {
            // Use RAM B-Tree index for efficient sorting
            TraceEntry min_key, max_key;
            min_key.id = 0; 
            max_key.id = INT_MAX;
            vector<TraceEntry> idx = ram_tree->range_search(min_key, max_key);
            
            for (const auto& entry : idx) {
                int real_id = (int)entry.ram_usage; // Real ID stored in ram_usage field
                TraceEntry target; 
                target.id = real_id;
                auto rec = main_tree->range_search(target, target);
                if (!rec.empty() && rec[0].project_id == project_id) {
                    results.push_back(rec[0]);
                }
            }
        } else {
            // Default: sort by ID (chronological order)
            results = query_range(project_id, 0, INT_MAX);
        }
        
        // Apply order (reverse if desc)
        if (order == "desc") {
            reverse(results.begin(), results.end());
        }
        
        // Apply pagination
        int start = min(offset, (int)results.size());
        int end = min(offset + limit, (int)results.size());
        
        return vector<TraceEntry>(results.begin() + start, results.begin() + end);
    }

    // Aggregated statistics structure
    struct AggregatedStats {
        uint64_t avg_duration = 0;
        uint64_t max_ram = 0;
        uint64_t min_duration = ULLONG_MAX;
        int count = 0;
        uint64_t total_duration = 0; // For computing average
    };

    // Compute Statistics: Aggregate metrics by function or version
    unordered_map<string, AggregatedStats> compute_stats(int project_id, string group_by)
    {
        lock_guard<mutex> lock(db_mutex);
        unordered_map<string, AggregatedStats> stats;
        
        auto logs = query_range(project_id, 0, INT_MAX);
        
        for (const auto& entry : logs) {
            string key = (group_by == "func") ? string(entry.process_name) : string(entry.app_version);
            
            // Skip empty keys
            if (key.empty()) continue;
            
            stats[key].count++;
            stats[key].total_duration += entry.duration;
            stats[key].max_ram = max(stats[key].max_ram, entry.ram_usage);
            stats[key].min_duration = min(stats[key].min_duration, entry.duration);
        }
        
        // Compute averages
        for (auto& [key, stat] : stats) {
            if (stat.count > 0) {
                stat.avg_duration = stat.total_duration / stat.count;
            }
        }
        
        return stats;
    }

    void debug_dump()
    {
        lock_guard<mutex> lock(db_mutex);
        main_tree->print_tree();
    }
};