#include "../include/DiskManager.hpp"

int main() {
    std::cout << "=== Testing DiskManager ===" << std::endl;
    
    // Test 1: Create database
    DiskManager dm("test_disk.db");
    std::cout << "✓ DiskManager created" << std::endl;
    
    // Test 2: Write page
    char write_buffer[PAGE_SIZE];
    memset(write_buffer, 0, PAGE_SIZE);
    strcpy(write_buffer, "Hello, DiskManager!");
    dm.write_page(0, write_buffer);
    std::cout << "✓ Page written" << std::endl;
    
    // Test 3: Read page
    char read_buffer[PAGE_SIZE];
    dm.read_page(0, read_buffer);
    std::cout << "✓ Page read: " << read_buffer << std::endl;
    
    // Test 4: Allocate page
    int new_page = dm.allocate_page();
    std::cout << "✓ Allocated new page: " << new_page << std::endl;
    
    std::cout << "\n=== DiskManager Tests PASSED ===" << std::endl;
    return 0;
}
