#pragma once
#include "DiskManager.hpp"
#include "Node.hpp"

class BTree {
private:
    DiskManager* dm;
    int root_page_id;

    // Splits a full child node into two
    void split_child(int parent_id, int index, int child_id);
    
    // helper to insert into a node
    void insert_non_full(int page_id, int key);

public:
    // Constructor
    BTree(DiskManager* disk_manager);

    // Main insert function
    void insert(int key);

    // Search functions
    bool search(int key, int page_id = -1);
    vector<int> range_search(int start_key, int end_key, int page_id = -1);
    
    // Debug helper
    void print_tree(int page_id = -1, int level = 0);
};
