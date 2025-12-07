#pragma once
#include <vector>
#include <iostream>
#include <cstring> 
#include "Types.hpp"

using namespace std;

struct Node {
    int page_id;
    bool is_leaf;
    
    vector<int> keys;
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

        int num_keys = keys.size();
        memcpy(data + off, &num_keys, sizeof(int));
        off += sizeof(int);

        for (int i = 0; i < num_keys; i++) 
        {
            memcpy(data + off, &keys[i], sizeof(int));
            off += sizeof(int);
        }

        int num_children = children.size();
        for (int i = 0; i < num_children; i++) 
        {
            memcpy(data + off, &children[i], sizeof(int));
            off += sizeof(int);
        }
    }

    // Load from buffer
    void deserialize(char* data) {
        int off = 0;

        memcpy(&is_leaf, data + off, sizeof(bool));
        off += sizeof(bool);

        int num_keys = 0;
        memcpy(&num_keys, data + off, sizeof(int));
        off += sizeof(int);

        keys.clear(); 
        for (int i = 0; i < num_keys; i++) 
        {
            int key;
            memcpy(&key, data + off, sizeof(int));
            keys.push_back(key);
            off += sizeof(int);
        }

        children.clear();
        if (!is_leaf) 
        {
            int num_children = num_keys + 1; 
            for (int i = 0; i < num_children; i++) 
            {
                int child_id;
                memcpy(&child_id, data + off, sizeof(int));
                children.push_back(child_id);
                off += sizeof(int);
            }
        }
    }

    void print_node() 
    {
        cout << "Node ID: " << page_id << " [Leaf: " << (is_leaf ? "Yes" : "No") << "]" << endl;
        cout << "Keys: ";
        for (int k : keys) cout << k << " ";
        cout << endl;
        if (!children.empty()) {
            cout << "Children: ";
            for (int c : children) cout << c << " ";
            cout << endl;
        }
    }
};