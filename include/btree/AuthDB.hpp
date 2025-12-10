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

using namespace std;

class AuthDB
{
private:
    DiskManager* dm;
    BTree<UserEntry>* tree;
    mutex auth_mutex; 

    string generate_api_key()
    {
        string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        string key = "";

        for (int i = 0; i < 32; i++)
        {
            int index = rand() % chars.length();
            key += chars[index];
        }

        return key;
    }

public:
    AuthDB(string filename)
    {
        dm = new DiskManager(filename);
        tree = new BTree<UserEntry>(dm);
    }

    ~AuthDB()
    {
        delete tree;
        delete dm;
    }

    string register_user(string username)
    {
        lock_guard<mutex> lock(auth_mutex); 

        static int id_counter = 1; 
        
        string new_key = generate_api_key();
        
        UserEntry user(new_key.c_str(), id_counter++, username.c_str());
        tree->insert(user);
        
        return new_key;
    }

    bool validate_key(string api_key, UserEntry& out_user)
    {
        lock_guard<mutex> lock(auth_mutex); 

        UserEntry target;
        strncpy(target.api_key, api_key.c_str(), 63);
        
        vector<UserEntry> result = tree->range_search(target, target);
        
        if (!result.empty())
        {
            out_user = result[0];
            return true;
        }
        return false;
    }

    void list_users()
    {
        lock_guard<mutex> lock(auth_mutex);
        tree->print_tree();
    }
};