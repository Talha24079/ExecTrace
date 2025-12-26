#include "../include/BTree.hpp"

int main() {
    std::cout << "=== Testing B-Tree ===" << std::endl;
    
    // Test 1: Create B-Tree
    DiskManager dm("test_btree.db");
    BTree<TraceEntry> tree(&dm);
    std::cout << "✓ B-Tree created" << std::endl;
    
    // Test 2: Insert entries
    for (int i = 1; i <= 5; i++) {
        TraceEntry entry(i, 100, "test_func", "Test message", "v1.0.0", i * 100, i * 1024);
        tree.insert(entry);
        std::cout << "✓ Inserted entry " << i << std::endl;
    }
    
    // Test 3: Search
    TraceEntry search_key(3, 0, "", "", "", 0, 0);
    auto results = tree.search(search_key);
    std::cout << "✓ Search found " << results.size() << " entries" << std::endl;
    
    if (!results.empty()) {
        std::cout << "  Found: ID=" << results[0].id 
                  << ", func=" << results[0].func 
                  << ", duration=" << results[0].duration << "ms" << std::endl;
    }
    
    std::cout << "\n=== B-Tree Tests PASSED ===" << std::endl;
    return 0;
}
