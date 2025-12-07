#pragma once
#include <string>
#include <cstdint>

using namespace std;

class Hasher
{
public:
    static uint64_t hash_string(const string& str)
    {
        uint64_t hash = 0;
        uint64_t prime = 31;

        for (char c : str)
        {
            hash = hash * prime + c;
        }
        return hash;
    }

    static uint64_t generate_unique_id(const string& func_name, uint64_t timestamp)
    {
        uint64_t func_hash = hash_string(func_name);
        return (timestamp << 16) ^ func_hash;
    }
};