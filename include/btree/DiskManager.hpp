#pragma once
#include <fstream>
#include <string>
#include <iostream>
#include <mutex>
#include "Types.hpp"

using namespace std;

class DiskManager {
private:
    fstream db_file;     // file object
    string file_name;    // Name of the database file
    int next_page_id;    // Keeps track of the next available ID
    mutable std::mutex file_mutex;  // Thread safety for file operations

public:
    // Constructor
    DiskManager(string filename);

    // Destructor 
    ~DiskManager();

    // Core functions
    void write_page(int page_id, const char* data);
    void read_page(int page_id, char* data);
    
    // helper
    int allocate_page();
};
