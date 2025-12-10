#pragma once
#include <vector>
#include <cstring>
#include "Types.hpp" 

using namespace std;

template <typename T>
class Node
{
public:
    int page_id;
    bool is_leaf;
    vector<T> entries;
    vector<int> children;

    static const int CAPACITY = (PAGE_SIZE - sizeof(bool) - 2 * sizeof(int)) / (sizeof(T) + sizeof(int));
    
    static const int DEGREE = (CAPACITY + 1) / 2;
    static const int MAX_KEYS = 2 * DEGREE - 1;

    Node(int id, bool leaf) : page_id(id), is_leaf(leaf) {}

    Node() : page_id(0), is_leaf(true) {}

    void serialize(char* buffer)
    {
        memset(buffer, 0, PAGE_SIZE);
        int offset = 0;

        memcpy(buffer + offset, &is_leaf, sizeof(bool));
        offset += sizeof(bool);

        int count = entries.size();
        memcpy(buffer + offset, &count, sizeof(int));
        offset += sizeof(int);

        if (count > 0)
        {
            memcpy(buffer + offset, entries.data(), count * sizeof(T));
            offset += count * sizeof(T);
        }

        if (!is_leaf)
        {
            memcpy(buffer + offset, children.data(), children.size() * sizeof(int));
        }
    }

    void deserialize(char* buffer)
    {
        int offset = 0;

        memcpy(&is_leaf, buffer + offset, sizeof(bool));
        offset += sizeof(bool);

        int count;
        memcpy(&count, buffer + offset, sizeof(int));
        offset += sizeof(int);

        entries.resize(count);
        if (count > 0)
        {
            memcpy(entries.data(), buffer + offset, count * sizeof(T));
            offset += count * sizeof(T);
        }

        if (!is_leaf)
        {
            children.resize(count + 1);
            memcpy(children.data(), buffer + offset, (count + 1) * sizeof(int));
        }
    }
};