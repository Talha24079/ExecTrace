#pragma once
#include <vector>
#include <iostream>
#include "Types.hpp"
using namespace std;

struct Node {
    int page_id;       
    bool is_leaf;      
    
    vector<int> keys;
    vector<int> children;

    Node(int id, bool leaf) 
    {
        page_id = id;
        is_leaf = leaf;
    }

    void print_node() 
    {
        cout << "Node ID: " << page_id << " [Leaf: " << is_leaf << "]" << endl;
        cout << "Keys: ";
        for (int k : keys) 
            cout << k << " ";
        cout << endl;
    }
};
