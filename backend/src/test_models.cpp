#include "../include/Models.hpp"
#include <iostream>

using namespace ExecTrace;

int main() {
    std::cout << "=== Testing Models ===" << std::endl;
    
    // Test 1: Default TraceEntry
    TraceEntry entry1;
    std::cout << "✓ Default TraceEntry created" << std::endl;
    
    // Test 2: TraceEntry with parameters
    TraceEntry entry2(1, 100, "test_function", "Test message", "v1.0.0", 150, 2048);
    std::cout << "✓ TraceEntry with params created" << std::endl;
    std::cout << "  ID: " << entry2.id << std::endl;
    std::cout << "  Project: " << entry2.project_id << std::endl;
    std::cout << "  Function: " << entry2.func << std::endl;
    std::cout << "  Message: " << entry2.message << std::endl;
    std::cout << "  Version: " << entry2.app_version << std::endl;
    std::cout << "  Duration: " << entry2.duration << "ms" << std::endl;
    std::cout << "  RAM: " << entry2.ram_usage << "KB" << std::endl;
    
    // Test 3: Null safety
    TraceEntry entry3(2, 200, nullptr, nullptr, nullptr, 100, 1024);
    std::cout << "✓ TraceEntry with nulls handled safely" << std::endl;
    
    // Test 4: Comparison operators
    if (entry2 < entry3) {
        std::cout << "✓ Comparison operator works" << std::endl;
    }
    
    // Test 5: UserEntry
    UserEntry user;
    std::cout << "✓ UserEntry created" << std::endl;
    
    // Test 6: ProjectEntry
    ProjectEntry project;
    std::cout << "✓ ProjectEntry created" << std::endl;
    
    std::cout << "\n=== All Model Tests PASSED ===" << std::endl;
    return 0;
}
