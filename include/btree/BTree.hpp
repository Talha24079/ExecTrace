#pragma once
#include "DiskManager.hpp"
#include "Node.hpp"
#include "TraceEntry.hpp"

class BTree {
private:
    DiskManager* dm;
    int root_page_id;

    // Splits a full child node into two
    void split_child(int parent_id, int index, int child_id);
    
    // helper to insert into a node
    void insert_non_full(int page_id, TraceEntry entry);

    void _remove(int page_id, int key);        
    TraceEntry get_predecessor(int page_id);          
    TraceEntry get_successor(int page_id);            
    void fill(int page_id, int idx);           
    void borrow_from_prev(int page_id, int idx);
    void borrow_from_next(int page_id, int idx);
    void merge(int page_id, int idx);

public:
    // Constructor
    BTree(DiskManager* disk_manager);

    // Main insert function
    void insert(TraceEntry key);

    // Main delete function
    void remove(int key);

    // Search functions
    bool search(int key, int page_id = -1);
    vector<TraceEntry> range_search(int start_key, int end_key, int page_id = -1);
    
    // Debug helper
    void print_tree(int page_id = -1, int level = 0);
};
