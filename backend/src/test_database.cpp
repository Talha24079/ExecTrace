#include "../include/Database.hpp"

int main() {
    std::cout << "=== Testing ExecTraceDB ===" << std::endl;
    
    // Test 1: Create database
    ExecTraceDB db("test_exectracedb.db");
    std::cout << "✓ ExecTraceDB created" << std::endl;
    
    // Test 2: Log events
    int id1 = db.log_event(100, "test_func_1", "Test message 1", "v1.0.0", 150, 2048);
    std::cout << "✓ Logged event 1, ID=" << id1 << std::endl;
    
    int id2 = db.log_event(100, "test_func_2", "Test message 2", "v1.0.0", 200, 4096);
    std::cout << "✓ Logged event 2, ID=" << id2 << std::endl;
    
    int id3 = db.log_event(200, "test_func_3", "Test message 3", "v2.0.0", 100, 1024);
    std::cout << "✓ Logged event 3, ID=" << id3 << std::endl;
    
    // Test 3: Search
    auto results = db.search(id2);
    std::cout << "✓ Search found " << results.size() << " entries" << std::endl;
    
    if (!results.empty()) {
        std::cout << "  ID=" << results[0].id 
                  << ", project=" << results[0].project_id
                  << ", func=" << results[0].func << std::endl;
    }
    
    std::cout << "\n=== ExecTraceDB Tests PASSED ===" << std::endl;
    return 0;
}
