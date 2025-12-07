#include <iostream>
#include "../include/btree/DiskManager.hpp"
#include "../include/btree/BTree.hpp"

using namespace std;

int main() {
    cout << "--- B-Tree Insertion Test ---" << endl;

    remove("btree.db"); 

    DiskManager dm("btree.db");
    BTree tree(&dm);

    cout << "Inserting keys 10 to 200..." << endl;
    for (int i = 1; i <= 20; i++) 
    {
        int key = i * 10;
        tree.insert(key);
        cout << "Inserted " << key << endl;
    }

    cout << "\n--- Final Tree Structure ---" << endl;
    tree.print_tree();

    tree.insert(210);
    cout << "\nInserted 210\n";
    tree.print_tree();

    return 0;
}
