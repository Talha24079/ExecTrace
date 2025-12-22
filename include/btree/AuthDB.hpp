#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <random>
#include <algorithm>
#include <mutex>
#include "DiskManager.hpp"
#include "BTree.hpp"
#include "../include/auth/UserEntry.hpp"
#include "../include/auth/ProjectEntry.hpp" 

using namespace std;

class AuthDB
{
private:
    DiskManager* dm_users;
    DiskManager* dm_projects;
    BTree<UserEntry>* user_tree;
    BTree<ProjectEntry>* project_tree;
    mutex auth_mutex;
    int next_user_id;     // Persistent counter
    int next_project_id;  // Persistent counter

    string generate_api_key(const string& prefix = "sk_")
    {
        string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        string key = prefix;

        for (int i = 0; i < 32; i++)
        {
            int index = rand() % chars.length();
            key += chars[index];
        }
        return key;
    }

public:
    AuthDB(string user_file, string proj_file)
    {
        dm_users = new DiskManager(user_file);
        dm_projects = new DiskManager(proj_file);
        
        user_tree = new BTree<UserEntry>(dm_users);
        project_tree = new BTree<ProjectEntry>(dm_projects);
        
        // Initialize counters by scanning existing B-Trees
        next_user_id = 1;
        next_project_id = 1;
        
        // Scan user tree to find max user_id
        UserEntry min_u, max_u;
        memset(min_u.api_key, 0, 64);
        memset(max_u.api_key, 0xFF, 64);
        vector<UserEntry> all_users = user_tree->range_search(min_u, max_u);
        for(const auto& u : all_users) {
            if(u.user_id >= next_user_id) {
                next_user_id = u.user_id + 1;
            }
        }
        
        // Scan project tree to find max project_id
        ProjectEntry min_p, max_p;
        memset(min_p.api_key, 0, 64);
        memset(max_p.api_key, 0xFF, 64);
        vector<ProjectEntry> all_projects = project_tree->range_search(min_p, max_p);
        for(const auto& p : all_projects) {
            if(p.project_id >= next_project_id) {
                next_project_id = p.project_id + 1;
            }
        }
    }

    ~AuthDB()
    {
        delete user_tree;
        delete project_tree;
        delete dm_users;
        delete dm_projects;
    }

    string register_user(string username)
    {
        lock_guard<mutex> lock(auth_mutex);
        string new_key = generate_api_key("usr_");
        
        UserEntry user(new_key.c_str(), next_user_id++, username.c_str());
        user_tree->insert(user);
        return new_key;
    }

    bool validate_user_key(string api_key, UserEntry& out_user)
    {
        lock_guard<mutex> lock(auth_mutex);
        UserEntry target;
        strncpy(target.api_key, api_key.c_str(), 63);
        
        vector<UserEntry> result = user_tree->range_search(target, target);
        if (!result.empty())
        {
            out_user = result[0];
            return true;
        }
        return false;
    }

    string create_project(int owner_id, string project_name)
    {
        lock_guard<mutex> lock(auth_mutex);
        
        string new_key = generate_api_key("sk_live_");
        
        ProjectEntry proj(new_key.c_str(), next_project_id++, owner_id, project_name.c_str());
        project_tree->insert(proj);
        
        return new_key;
    }

    bool validate_project_key(string api_key, ProjectEntry& out_proj)
    {
        lock_guard<mutex> lock(auth_mutex);
        ProjectEntry target;
        strncpy(target.api_key, api_key.c_str(), 63);

        vector<ProjectEntry> result = project_tree->range_search(target, target);
        if (!result.empty())
        {
            out_proj = result[0];
            return true;
        }
        return false;
    }

    vector<ProjectEntry> get_user_projects(int user_id)
    {
        lock_guard<mutex> lock(auth_mutex);
        
        ProjectEntry min_p, max_p;
        memset(min_p.api_key, 0, 64);
        memset(max_p.api_key, 0xFF, 64);
        
        vector<ProjectEntry> all = project_tree->range_search(min_p, max_p);
        vector<ProjectEntry> filtered;
        
        for(const auto& p : all)
        {
            if(p.owner_id == user_id)
                filtered.push_back(p);
        }
        return filtered;
    }

    // Delete a project (returns true if successful)
    bool delete_project(string api_key, int owner_id)
    {
        lock_guard<mutex> lock(auth_mutex);
        ProjectEntry target;
        strncpy(target.api_key, api_key.c_str(), 63);
        target.api_key[63] = '\0';
        
        vector<ProjectEntry> result = project_tree->range_search(target, target);
        if (result.empty()) return false;
        
        // Verify ownership
        if (result[0].owner_id != owner_id) return false;
        
        // Delete from B-Tree
        project_tree->remove(result[0]);
        return true;
    }

    // Rename a project (returns true if successful)
    bool rename_project(string api_key, int owner_id, string new_name)
    {
        lock_guard<mutex> lock(auth_mutex);
        
        // Validate new name
        if (new_name.empty() || new_name.length() > 31) return false;
        
        ProjectEntry target;
        strncpy(target.api_key, api_key.c_str(), 63);
        target.api_key[63] = '\0';
        
        vector<ProjectEntry> result = project_tree->range_search(target, target);
        if (result.empty()) return false;
        
        // Verify ownership
        if (result[0].owner_id != owner_id) return false;
        
        // Update name
        ProjectEntry updated = result[0];
        strncpy(updated.name, new_name.c_str(), 31);
        updated.name[31] = '\0';
        
        // Replace in B-Tree (remove old, insert updated)
        project_tree->remove(result[0]);
        project_tree->insert(updated);
        return true;
    }

    // Regenerate API key for a project (returns new key or empty string on failure)
    string regenerate_project_key(string old_api_key, int owner_id)
    {
        lock_guard<mutex> lock(auth_mutex);
        
        ProjectEntry target;
        strncpy(target.api_key, old_api_key.c_str(), 63);
        target.api_key[63] = '\0';
        
        vector<ProjectEntry> result = project_tree->range_search(target, target);
        if (result.empty()) return "";
        
        // Verify ownership
        if (result[0].owner_id != owner_id) return "";
        
        // Generate new key
        string new_key = generate_api_key("sk_live_");
        
        // Update entry with new key
        ProjectEntry updated = result[0];
        strncpy(updated.api_key, new_key.c_str(), 63);
        updated.api_key[63] = '\0';
        
        // Replace in B-Tree (remove old, insert new)
        project_tree->remove(result[0]);
        project_tree->insert(updated);
        
        return new_key;
    }
};