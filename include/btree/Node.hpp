#pragma once
#include <vector>
#include <iostream>
#include <cstring> 
#include "Types.hpp"
#include "TraceEntry.hpp" // Include your new struct

using namespace std;

struct Node {
    int page_id;
    bool is_leaf;
    vector<TraceEntry> entries; 
    vector<int> children;

    // Constructor
    Node(int id, bool leaf) {
        page_id = id;
        is_leaf = leaf;
    }

    // Save to buffer 
    void serialize(char* data) 
    {
        int off = 0;

        memcpy(data + off, &is_leaf, sizeof(bool));
        off += sizeof(bool);

        int num_entries = entries.size();
        memcpy(data + off, &num_entries, sizeof(int));
        off += sizeof(int);

        for (int i = 0; i < num_entries; i++) 
        {
            memcpy(data + off, &entries[i], sizeof(TraceEntry));
            off += sizeof(TraceEntry);
        }

        int num_children = children.size();
        memcpy(data + off, &num_children, sizeof(int));
        off += sizeof(int);

        for (int i = 0; i < num_children; i++) 
        {
            memcpy(data + off, &children[i], sizeof(int));
            off += sizeof(int);
        }
    }
    void deserialize(char* data) 
    {
        int off = 0;

        memcpy(&is_leaf, data + off, sizeof(bool));
        off += sizeof(bool);

        int num_entries = 0;
        memcpy(&num_entries, data + off, sizeof(int));
        off += sizeof(int);

        entries.clear(); 
        for (int i = 0; i < num_entries; i++) 
        {
            TraceEntry entry;
            memcpy(&entry, data + off, sizeof(TraceEntry));
            entries.push_back(entry);
            off += sizeof(TraceEntry);
        }

        children.clear();
        int num_children = 0;
        memcpy(&num_children, data + off, sizeof(int));
        off += sizeof(int);

        for (int i = 0; i < num_children; i++) 
        {
            int child_id;
            memcpy(&child_id, data + off, sizeof(int));
            children.push_back(child_id);
            off += sizeof(int);
        }
    }

    void print_node() 
    {
        cout << "Node ID: " << page_id << " [Leaf: " << (is_leaf ? "Yes" : "No") << "]" << endl;
        cout << "Keys: ";
        for (const auto& e : entries) cout << e.id << " ";
        cout << endl;
        
        if (!children.empty()) {
            cout << "Children: ";
            for (int c : children) cout << c << " ";
            cout << endl;
        }
    }
};