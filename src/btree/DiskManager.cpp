#include "../../include/btree/DiskManager.hpp"
#include <cstring>

DiskManager::DiskManager(string filename)
{
    file_name = filename;
    // Open file and determine existing number of pages
    db_file.open(file_name, ios::in | ios::out | ios::binary);

    if (!db_file.is_open()) 
    {
        // create empty file
        db_file.clear();
        db_file.open(file_name, ios::out | ios::binary | ios::trunc);
        db_file.close();
        db_file.open(file_name, ios::in | ios::out | ios::binary);
    }

    // determine current file size and set next_page_id
    db_file.seekg(0, ios::end);
    std::streampos size = db_file.tellg();
    if (size <= 0) next_page_id = 0;
    else next_page_id = static_cast<int>(size / PAGE_SIZE);
}

DiskManager::~DiskManager() 
{
    if (db_file.is_open()) 
        db_file.close();
}


void DiskManager::write_page(int page_id, const char* data) 
{
    std::lock_guard<std::mutex> lock(file_mutex);
    int offset = page_id * PAGE_SIZE;
    // ensure file is open
    if (!db_file.is_open()) {
        db_file.open(file_name, ios::in | ios::out | ios::binary);
        if (!db_file.is_open()) {
            // attempt to create the file
            db_file.clear();
            db_file.open(file_name, ios::out | ios::binary | ios::trunc);
            db_file.close();
            db_file.open(file_name, ios::in | ios::out | ios::binary);
        }
    }

    db_file.seekp(offset);
    if (!db_file.good()) {
        db_file.clear();
    }

    db_file.write(data, PAGE_SIZE);
    db_file.flush();

    if (!db_file.good()) {
        cerr << "DiskManager::write_page: write failed for page " << page_id << "\n";
    }

    // Ensure allocation state reflects pages written directly
    if (page_id >= next_page_id) {
        next_page_id = page_id + 1;
    }
}

void DiskManager::read_page(int page_id, char* data) 
{
    std::lock_guard<std::mutex> lock(file_mutex);
    int offset = page_id * PAGE_SIZE;

    // initialize buffer to zeros to avoid using uninitialized data
    memset(data, 0, PAGE_SIZE);

    db_file.seekg(offset);
    if (!db_file.good()) return; // nothing to read

    db_file.read(data, PAGE_SIZE);
}

int DiskManager::allocate_page() 
{
    std::lock_guard<std::mutex> lock(file_mutex);
    int id = next_page_id;
    next_page_id++;
    return id;
}
