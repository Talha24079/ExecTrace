#include <iostream>
#include <cstring>
#include "btree/DiskManager.hpp"
#include "btree/Node.hpp"

using namespace std;

int main() {
    cout << "--- Serialization Test ---" << endl;

    string db_file = "btree.db";
    DiskManager dm(db_file);

    cout << "1. Creating Node 0 (Root)..." << endl;
    Node originalNode(0, true); 
    originalNode.keys.push_back(100);
    originalNode.keys.push_back(200);
    originalNode.keys.push_back(300);
    
    cout << "Original Node:" << endl;
    originalNode.print_node();

    char buffer[PAGE_SIZE];
    memset(buffer, 0, PAGE_SIZE); 
    originalNode.serialize(buffer);

    dm.write_page(0, buffer);
    cout << "2. Wrote Node 0 to disk." << endl;

    char readBuffer[PAGE_SIZE];
    memset(readBuffer, 0, PAGE_SIZE);
    dm.read_page(0, readBuffer);

    Node loadedNode(0, true); 
    loadedNode.deserialize(readBuffer);

    cout << "3. Loaded Node from disk:" << endl;
    loadedNode.print_node();

    if (loadedNode.keys == originalNode.keys) 
    {
        cout << "SUCCESS: Data matches!" << endl;
    } 
    else 
    {
        cout << "FAILURE: Data does not match." << endl;
    }

    return 0;
}