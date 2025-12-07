#include <iostream>
#include <cstring> 
#include "btree/DiskManager.hpp"

using namespace std;

int main() {
    cout << "--- Disk Manager Test ---" << endl;

    string db_file = "test.db";
    DiskManager dm(db_file);

    char buffer[PAGE_SIZE];
    
    memset(buffer, 0, PAGE_SIZE);

    string message = "Hello, this is saved on disk!";
    strcpy(buffer, message.c_str());

    cout << "Writing to Page 0..." << endl;
    dm.write_page(0, buffer);

    memset(buffer, 0, PAGE_SIZE);

    cout << "Reading from Page 0..." << endl;
    dm.read_page(0, buffer);

    cout << "Data read: " << buffer << endl;

    return 0;
}