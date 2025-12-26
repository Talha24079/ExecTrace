#pragma once
#include "BTree.hpp"

class ExecTraceDB {
private:
    DiskManager* dm;
    BTree<ExecTrace::TraceEntry>* trace_tree;
    std::mutex db_mutex;
    int next_id;

public:
    ExecTraceDB(const std::string& db_file) : next_id(1) {
        dm = new DiskManager(db_file);
        trace_tree = new BTree<ExecTrace::TraceEntry>(dm);
        std::cout << "[ExecTraceDB] Initialized traces database" << std::endl;
    }

    ~ExecTraceDB() {
        delete trace_tree;
        delete dm;
    }

    int log_event(int project_id, const char* func, const char* msg, 
                  const char* app_version, uint64_t duration, uint64_t ram) {
        std::lock_guard<std::mutex> lock(db_mutex);
        
        int entry_id = next_id++;
        ExecTrace::TraceEntry entry(entry_id, project_id, func, msg, app_version, duration, ram);
        trace_tree->insert(entry);
        
        std::cout << "[TraceDB] Logged event " << entry_id << " for project " << project_id 
                  << ": " << func << " (" << duration << "ms)" << std::endl;
        
        return entry_id;
    }

    std::vector<ExecTrace::TraceEntry> search(int entry_id) {
        std::lock_guard<std::mutex> lock(db_mutex);
        ExecTrace::TraceEntry search_key(entry_id, 0, "", "", "", 0, 0);
        return trace_tree->search(search_key);
    }

    std::vector<ExecTrace::TraceEntry> search_by_project(int project_id) {
        std::lock_guard<std::mutex> lock(db_mutex);
        std::cout << "[ExecTraceDB] Searching traces for project " << project_id << std::endl;
        
        // Get all traces and filter by project_id
        auto all_traces = trace_tree->get_all_values();
        std::vector<ExecTrace::TraceEntry> filtered;
        
        for (const auto& entry : all_traces) {
            if (entry.project_id == project_id) {
                filtered.push_back(entry);
            }
        }
        
        std::cout << "[ExecTraceDB] Found " << filtered.size() << " traces for project " << project_id << std::endl;
        return filtered;
    }

    // Get all traces (for demo/testing)
    std::vector<ExecTrace::TraceEntry> get_all_traces() {
        std::lock_guard<std::mutex> lock(db_mutex);
        std::cout << "[ExecTraceDB] get_all_traces called" << std::endl;
        
        // Use B-Tree traversal to get all entries
        auto all_entries = trace_tree->get_all_values();
        std::cout << "[ExecTraceDB] Found " << all_entries.size() << " total entries" << std::endl;
        
        return all_entries;
    }
};
