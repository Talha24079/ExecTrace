#pragma once
#include <cstring>
#include <iostream>
#include <iomanip>
using namespace std;

struct UserEntry {
    char api_key[64];   
    int user_id;        
    char username[32];  
    long created_at;    

    UserEntry() : user_id(0), created_at(0) 
    {
        memset(api_key, 0, sizeof(api_key));
        memset(username, 0, sizeof(username));
    }

    UserEntry(const char* key, int id, const char* name) 
    {
        strncpy(api_key, key, sizeof(api_key) - 1);
        api_key[sizeof(api_key) - 1] = '\0'; 

        user_id = id;
        
        strncpy(username, name, sizeof(username) - 1);
        username[sizeof(username) - 1] = '\0';

        created_at = time(nullptr);
    }
    
    bool operator<(const UserEntry& other) const {
        return strcmp(api_key, other.api_key) < 0;
    }

    bool operator>(const UserEntry& other) const {
        return strcmp(api_key, other.api_key) > 0;
    }

    bool operator==(const UserEntry& other) const {
        return strcmp(api_key, other.api_key) == 0;
    }

    void print() const {
        cout << "[User] ID: " << user_id 
                  << " | Key: " << api_key 
                  << " | Name: " << username << endl;
    }
};