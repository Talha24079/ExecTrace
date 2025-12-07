#include "../../include/btree/DiskManager.hpp"

// Constructor: Opens the file in binary mode
DiskManager::DiskManager(string filename) {
    file_name = filename;
    next_page_id = 0;

    db_file.open(file_name, ios::in | ios::out | ios::binary);

    if (!db_file.is_open()) 
    {
        db_file.clear();
        db_file.open(file_name, ios::out | ios::binary | ios::trunc);
        db_file.close();
        db_file.open(file_name, ios::in | ios::out | ios::binary);
    }
}

DiskManager::~DiskManager() 
{
    if (db_file.is_open()) 
        db_file.close();
}


void DiskManager::write_page(int page_id, const char* data) 
{
    int offset = page_id * PAGE_SIZE;
    
    db_file.seekp(offset); 
    db_file.write(data, PAGE_SIZE);
    db_file.flush();       
}

void DiskManager::read_page(int page_id, char* data) 
{
    int offset = page_id * PAGE_SIZE;

    db_file.seekg(offset); 
    db_file.read(data, PAGE_SIZE);
}

int DiskManager::allocate_page() 
{
    int id = next_page_id;
    next_page_id++;
    return id;
}
