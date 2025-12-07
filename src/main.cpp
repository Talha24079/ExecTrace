#include "btree/BTree.hpp"
#include <iostream>

int main() {
    DiskManager dm("btree_merge_real.db");
    BTree tree(&dm);

    // 1. Insert 6 keys (10, 20, 30, 40, 50, 60)
    // Root will split at 5 keys. 
    // Resulting Tree:
    // Root: [30] (or 40 depending on logic)
    // Left: [10, 20]
    // Right: [40, 50, 60]
    int keys[] = {10, 20, 30, 40, 50, 60};
    for (int k : keys) tree.insert(k);

    std::cout << "--- Initial Split Tree ---" << std::endl;
    tree.print_tree();

    // 2. Delete 60.
    // Right child becomes [40, 50].
    // Now BOTH children have 2 keys (Minimum allowed).
    tree.remove(60);
    std::cout << "\n--- After Deleting 60 (Setup) ---" << std::endl;
    tree.print_tree();

    // 3. Delete 40.
    // Right child becomes [50] (Underflow!).
    // Left child has [10, 20] (Cannot borrow, min is 2).
    // MUST MERGE: Left + Root(30) + Right.
    std::cout << "\n--- Deleting 40 (REAL MERGE TEST) ---" << std::endl;
    tree.remove(40);
    tree.print_tree();

    return 0;
}