#pragma once
#include <cstring>
#include <iostream>
#include <ctime>

using namespace std;

struct ProjectEntry
{
    char api_key[64];    
    int project_id;
    int owner_id;
    char name[32];       
    long created_at;

    ProjectEntry() : project_id(0), owner_id(0), created_at(0)
    {
        memset(api_key, 0, sizeof(api_key));
        memset(name, 0, sizeof(name));
    }

    ProjectEntry(const char* key, int p_id, int u_id, const char* p_name)
    {
        strncpy(api_key, key, sizeof(api_key) - 1);
        api_key[sizeof(api_key) - 1] = '\0';

        project_id = p_id;
        owner_id = u_id;

        strncpy(name, p_name, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';

        created_at = time(nullptr);
    }

    bool operator<(const ProjectEntry& other) const { return strcmp(api_key, other.api_key) < 0; }
    bool operator>(const ProjectEntry& other) const { return strcmp(api_key, other.api_key) > 0; }
    bool operator==(const ProjectEntry& other) const { return strcmp(api_key, other.api_key) == 0; }
    bool operator>=(const ProjectEntry& other) const { return strcmp(api_key, other.api_key) >= 0; }
    bool operator<=(const ProjectEntry& other) const { return strcmp(api_key, other.api_key) <= 0; }

    void print() const
    {
        cout << "[Project] ID: " << project_id
             << " | Owner: " << owner_id
             << " | Name: " << name
             << " | Key: " << api_key << endl;
    }
};