#include <iostream>
#include <vector>
#include "btree/DiskManager.hpp"
#include "btree/BTree.hpp"

using namespace std;

int main() {
    cout << "--- B-Tree Query Test ---" << endl;
    
    DiskManager dm("btree.db");
    BTree tree(&dm);

    cout << "\n1. Testing Search:" << endl;
    int search_target = 180;
    if (tree.search(search_target)) 
        cout << "Found key " << search_target << "!" << endl;
    else
        cout << "Key " << search_target << " NOT found (Error)." << endl;

    int missing_target = 999;
    if (!tree.search(missing_target))
        cout << "Correctly failed to find " << missing_target << "." << endl;

    cout << "\n2. Testing Range Query (65 to 115):" << endl;
    vector<int> range_results = tree.range_search(65, 115);
    
    cout << "Results: ";
    for (int k : range_results) 
        cout << k << " ";
    cout << endl;

    return 0;
}