#pragma once
#include <vector>
#include <iostream>
#include <string>
#include <cstring>
#include "DiskManager.hpp"
#include "Node.hpp"

using namespace std;

template <typename T>
class BTree
{
private:
    DiskManager* dm;
    int root_page_id;

    void split_child(int parent_id, int index, int child_id);
    void insert_non_full(int page_id, T entry);
    void _remove(int page_id, T key);
    T get_predecessor(int page_id);
    T get_successor(int page_id);
    void fill(int page_id, int idx);
    void borrow_from_prev(int page_id, int idx);
    void borrow_from_next(int page_id, int idx);
    void merge(int page_id, int idx);
    bool search(T key, int page_id);
    vector<T> range_search(T start_key, T end_key, int page_id);
    void print_tree(int page_id, int level);

public:
    BTree(DiskManager* disk_manager);
    void insert(T entry);
    void remove(T key);
    bool search(T key);
    vector<T> range_search(T start_key, T end_key);
    void print_tree();
};


template <typename T>
BTree<T>::BTree(DiskManager* disk_manager) : dm(disk_manager)
{
    root_page_id = 0;
    
    char buffer[PAGE_SIZE];
    dm->read_page(0, buffer);
    
    bool is_empty = true;
    for(int i = 0; i < PAGE_SIZE; i++)
    {
        if(buffer[i] != 0)
        {
            is_empty = false;
            break;
        }
    }

    if (is_empty)
    {
        Node<T> root(root_page_id, true);
        root.serialize(buffer);
        dm->write_page(root_page_id, buffer);
    }
}

template <typename T>
void BTree<T>::insert(T entry)
{
    char buffer[PAGE_SIZE];
    dm->read_page(root_page_id, buffer);
    Node<T> root(root_page_id, true);
    root.deserialize(buffer);

    if (root.entries.size() == Node<T>::MAX_KEYS)
    {
        int new_root_id = dm->allocate_page();
        Node<T> new_root(new_root_id, false);
        
        new_root.children.push_back(root_page_id);
        
        Node<T> old_root_moved = root; 
        old_root_moved.page_id = dm->allocate_page();
        
        root.entries.clear();
        root.children.clear();
        root.is_leaf = false;
        root.children.push_back(old_root_moved.page_id);

        char child_buf[PAGE_SIZE];
        old_root_moved.serialize(child_buf);
        dm->write_page(old_root_moved.page_id, child_buf);

        char root_buf[PAGE_SIZE];
        root.serialize(root_buf);
        dm->write_page(root_page_id, root_buf);

        split_child(root_page_id, 0, old_root_moved.page_id);
        insert_non_full(root_page_id, entry);
    }
    else
    {
        insert_non_full(root_page_id, entry);
    }
}

template <typename T>
void BTree<T>::split_child(int parent_id, int index, int child_id)
{
    char p_buf[PAGE_SIZE];
    dm->read_page(parent_id, p_buf);
    Node<T> parent(parent_id, false);
    parent.deserialize(p_buf);

    char c_buf[PAGE_SIZE];
    dm->read_page(child_id, c_buf);
    Node<T> child(child_id, true); 
    child.deserialize(c_buf);
    
    if (!child.children.empty()) child.is_leaf = false;

    int sibling_id = dm->allocate_page();
    Node<T> sibling(sibling_id, child.is_leaf);

    int degree = Node<T>::DEGREE;

    for (int j = 0; j < degree - 1; j++)
    {
        sibling.entries.push_back(child.entries[degree + j]);
    }

    if (!child.is_leaf)
    {
        for (int j = 0; j < degree; j++)
        {
            sibling.children.push_back(child.children[degree + j]);
        }
    }

    T median_entry = child.entries[degree - 1];

    child.entries.resize(degree - 1);
    if (!child.is_leaf)
        child.children.resize(degree);

    parent.children.insert(parent.children.begin() + index + 1, sibling_id);
    parent.entries.insert(parent.entries.begin() + index, median_entry);

    char buf[PAGE_SIZE];
    
    child.serialize(buf);
    dm->write_page(child_id, buf);

    sibling.serialize(buf);
    dm->write_page(sibling_id, buf);

    parent.serialize(buf);
    dm->write_page(parent_id, buf);
}

template <typename T>
void BTree<T>::insert_non_full(int page_id, T entry)
{
    char buffer[PAGE_SIZE];
    dm->read_page(page_id, buffer);
    Node<T> node(page_id, true); 
    node.deserialize(buffer);

    if (!node.children.empty()) node.is_leaf = false;

    int i = node.entries.size() - 1;

    if (node.is_leaf)
    {
        node.entries.push_back(T()); 
        while (i >= 0 && entry < node.entries[i])
        {
            node.entries[i + 1] = node.entries[i];
            i--;
        }
        node.entries[i + 1] = entry;
        
        node.serialize(buffer);
        dm->write_page(page_id, buffer);
    }
    else
    {
        while (i >= 0 && entry < node.entries[i])
            i--;

        i++;
        int child_id = node.children[i];
        
        char child_buf[PAGE_SIZE];
        dm->read_page(child_id, child_buf);
        Node<T> child(child_id, true);
        child.deserialize(child_buf);

        if (child.entries.size() == Node<T>::MAX_KEYS)
        {
            split_child(page_id, i, child_id);
            
            dm->read_page(page_id, buffer);
            node.entries.clear();
            node.children.clear();
            node.deserialize(buffer);
            if (!node.children.empty()) node.is_leaf = false;
            
            if (entry > node.entries[i])
                i++;
        }
        insert_non_full(node.children[i], entry);
    }
}

template <typename T>
void BTree<T>::remove(T key)
{
    if (root_page_id == -1) return;

    _remove(root_page_id, key);

    char buffer[PAGE_SIZE];
    dm->read_page(root_page_id, buffer);
    Node<T> root(root_page_id, true);
    root.deserialize(buffer);

    if (root.entries.empty())
    {
        if (root.is_leaf)
        {
            root_page_id = 0; 
            Node<T> empty_root(0, true);
            empty_root.serialize(buffer);
            dm->write_page(0, buffer);
        }
        else
        {
            root_page_id = root.children[0];
        }
    }
}

template <typename T>
void BTree<T>::_remove(int page_id, T key)
{
    char buffer[PAGE_SIZE];
    dm->read_page(page_id, buffer);
    Node<T> node(page_id, true);
    node.deserialize(buffer);

    int degree = Node<T>::DEGREE;

    int idx = 0;
    while (idx < node.entries.size() && node.entries[idx] < key)
    {
        idx++;
    }

    if (idx < node.entries.size() && node.entries[idx] == key)
    {
        if (node.is_leaf)
        {
            node.entries.erase(node.entries.begin() + idx);
            node.serialize(buffer);
            dm->write_page(page_id, buffer);
        }
        else
        {
            int left_child_id = node.children[idx];
            char left_buf[PAGE_SIZE];
            dm->read_page(left_child_id, left_buf);
            Node<T> left_child(left_child_id, true); 
            left_child.deserialize(left_buf);

            if (left_child.entries.size() >= degree)
            {
                T pred = get_predecessor(node.children[idx]);
                node.entries[idx] = pred;
                node.serialize(buffer);
                dm->write_page(page_id, buffer);
                _remove(node.children[idx], pred);
            }
            else
            {
                int right_child_id = node.children[idx + 1];
                char right_buf[PAGE_SIZE];
                dm->read_page(right_child_id, right_buf);
                Node<T> right_child(right_child_id, true); 
                right_child.deserialize(right_buf);

                if (right_child.entries.size() >= degree)
                {
                    T succ = get_successor(node.children[idx + 1]);
                    node.entries[idx] = succ;
                    node.serialize(buffer);
                    dm->write_page(page_id, buffer);
                    _remove(node.children[idx + 1], succ);
                }
                else
                {
                    merge(page_id, idx);
                    _remove(node.children[idx], key);
                }
            }
        }
    }
    else
    {
        if (node.is_leaf) return;

        bool flag = (idx == node.entries.size());

        int child_id = node.children[idx];
        char child_buf[PAGE_SIZE];
        dm->read_page(child_id, child_buf);
        Node<T> child_check(child_id, true); 
        child_check.deserialize(child_buf);

        if (child_check.entries.size() < degree)
        {
            fill(page_id, idx);
            dm->read_page(page_id, buffer);
            node.entries.clear();
            node.children.clear();
            node.deserialize(buffer);
        }

        if (flag && idx > node.entries.size())
        {
            _remove(node.children[idx - 1], key);
        }
        else
        {
            _remove(node.children[idx], key);
        }
    }
}

template <typename T>
T BTree<T>::get_predecessor(int page_id)
{
    char buf[PAGE_SIZE];
    dm->read_page(page_id, buf);
    Node<T> curr(page_id, true); 
    curr.deserialize(buf);

    while (!curr.is_leaf)
    {
        int next_page = curr.children.back();
        dm->read_page(next_page, buf);
        curr = Node<T>(next_page, true);
        curr.deserialize(buf);
    }
    return curr.entries.back();
}

template <typename T>
T BTree<T>::get_successor(int page_id)
{
    char buf[PAGE_SIZE];
    dm->read_page(page_id, buf);
    Node<T> curr(page_id, true); 
    curr.deserialize(buf);

    while (!curr.is_leaf)
    {
        int next_page = curr.children.front();
        dm->read_page(next_page, buf);
        curr = Node<T>(next_page, true);
        curr.deserialize(buf);
    }
    return curr.entries.front();
}

template <typename T>
void BTree<T>::fill(int page_id, int idx)
{
    int degree = Node<T>::DEGREE;

    char p_buf[PAGE_SIZE];
    dm->read_page(page_id, p_buf);
    Node<T> parent(page_id, false);
    parent.deserialize(p_buf);

    if (idx != 0)
    {
        int sibling_id = parent.children[idx - 1];
        char s_buf[PAGE_SIZE];
        dm->read_page(sibling_id, s_buf);
        Node<T> sibling(sibling_id, true); 
        sibling.deserialize(s_buf);
        
        if (sibling.entries.size() >= degree)
        {
            borrow_from_prev(page_id, idx);
            return;
        }
    }

    if (idx != parent.entries.size())
    {
        int sibling_id = parent.children[idx + 1];
        char s_buf[PAGE_SIZE];
        dm->read_page(sibling_id, s_buf);
        Node<T> sibling(sibling_id, true); 
        sibling.deserialize(s_buf);

        if (sibling.entries.size() >= degree)
        {
            borrow_from_next(page_id, idx);
            return;
        }
    }

    if (idx != parent.entries.size())
    {
        merge(page_id, idx);
    }
    else
    {
        merge(page_id, idx - 1);
    }
}

template <typename T>
void BTree<T>::borrow_from_prev(int page_id, int idx)
{
    char p_buf[PAGE_SIZE];
    dm->read_page(page_id, p_buf);
    Node<T> parent(page_id, false);
    parent.deserialize(p_buf);

    int child_id = parent.children[idx];
    int sibling_id = parent.children[idx - 1];

    char c_buf[PAGE_SIZE], s_buf[PAGE_SIZE];
    dm->read_page(child_id, c_buf);
    dm->read_page(sibling_id, s_buf);
    Node<T> child(child_id, true); child.deserialize(c_buf);
    Node<T> sibling(sibling_id, true); sibling.deserialize(s_buf);

    if (!child.children.empty()) child.is_leaf = false;
    if (!sibling.children.empty()) sibling.is_leaf = false;

    child.entries.insert(child.entries.begin(), parent.entries[idx - 1]);
    
    if (!child.is_leaf)
    {
        child.children.insert(child.children.begin(), sibling.children.back());
    }

    parent.entries[idx - 1] = sibling.entries.back();

    sibling.entries.pop_back();
    if (!sibling.is_leaf)
    {
        sibling.children.pop_back();
    }

    char buf[PAGE_SIZE];
    parent.serialize(buf); dm->write_page(page_id, buf);
    child.serialize(buf); dm->write_page(child_id, buf);
    sibling.serialize(buf); dm->write_page(sibling_id, buf);
}

template <typename T>
void BTree<T>::borrow_from_next(int page_id, int idx)
{
    char p_buf[PAGE_SIZE];
    dm->read_page(page_id, p_buf);
    Node<T> parent(page_id, false);
    parent.deserialize(p_buf);

    int child_id = parent.children[idx];
    int sibling_id = parent.children[idx + 1];

    char c_buf[PAGE_SIZE], s_buf[PAGE_SIZE];
    dm->read_page(child_id, c_buf);
    dm->read_page(sibling_id, s_buf);
    Node<T> child(child_id, true); child.deserialize(c_buf);
    Node<T> sibling(sibling_id, true); sibling.deserialize(s_buf);

    if (!child.children.empty()) child.is_leaf = false;
    if (!sibling.children.empty()) sibling.is_leaf = false;

    child.entries.push_back(parent.entries[idx]);

    if (!child.is_leaf)
    {
        child.children.push_back(sibling.children[0]);
    }

    parent.entries[idx] = sibling.entries[0];

    sibling.entries.erase(sibling.entries.begin());
    if (!sibling.is_leaf)
    {
        sibling.children.erase(sibling.children.begin());
    }

    char buf[PAGE_SIZE];
    parent.serialize(buf); dm->write_page(page_id, buf);
    child.serialize(buf); dm->write_page(child_id, buf);
    sibling.serialize(buf); dm->write_page(sibling_id, buf);
}

template <typename T>
void BTree<T>::merge(int page_id, int idx)
{
    char p_buf[PAGE_SIZE];
    dm->read_page(page_id, p_buf);
    Node<T> parent(page_id, false);
    parent.deserialize(p_buf);

    int child_id = parent.children[idx];
    int sibling_id = parent.children[idx + 1];

    char c_buf[PAGE_SIZE], s_buf[PAGE_SIZE];
    dm->read_page(child_id, c_buf);
    dm->read_page(sibling_id, s_buf);
    Node<T> child(child_id, true); child.deserialize(c_buf);
    Node<T> sibling(sibling_id, true); sibling.deserialize(s_buf);

    if (!child.children.empty()) child.is_leaf = false;
    if (!sibling.children.empty()) sibling.is_leaf = false;

    child.entries.push_back(parent.entries[idx]);

    for (const auto& e : sibling.entries) child.entries.push_back(e);

    if (!child.is_leaf)
    {
        for (int c : sibling.children) child.children.push_back(c);
    }

    parent.entries.erase(parent.entries.begin() + idx);
    parent.children.erase(parent.children.begin() + idx + 1);

    char buf[PAGE_SIZE];
    parent.serialize(buf); dm->write_page(page_id, buf);
    child.serialize(buf); dm->write_page(child_id, buf);
}

template <typename T>
bool BTree<T>::search(T key)
{
    return search(key, -1);
}

template <typename T>
bool BTree<T>::search(T key, int page_id)
{
    if (page_id == -1) page_id = root_page_id;

    char buffer[PAGE_SIZE];
    dm->read_page(page_id, buffer);
    Node<T> node(page_id, true);
    node.deserialize(buffer);

    int i = 0;
    while (i < node.entries.size() && key > node.entries[i])
    {
        i++;
    }

    if (i < node.entries.size() && key == node.entries[i])
    {
        return true;
    }

    if (node.is_leaf)
    {
        return false;
    }

    return search(key, node.children[i]);
}

template <typename T>
vector<T> BTree<T>::range_search(T start_key, T end_key)
{
    return range_search(start_key, end_key, -1);
}

template <typename T>
vector<T> BTree<T>::range_search(T start_key, T end_key, int page_id)
{
    if (page_id == -1) page_id = root_page_id;

    vector<T> results;
    char buffer[PAGE_SIZE];
    dm->read_page(page_id, buffer);
    Node<T> node(page_id, true);
    node.deserialize(buffer);

    int i = 0;
    
    if (!node.is_leaf)
    {
        for (i = 0; i < node.entries.size(); i++)
        {
            if (start_key <= node.entries[i])
            {
                vector<T> child_res = range_search(start_key, end_key, node.children[i]);
                results.insert(results.end(), child_res.begin(), child_res.end());
            }
            
            if (node.entries[i] >= start_key && node.entries[i] <= end_key)
            {
                results.push_back(node.entries[i]);
            }
        }
        if (end_key >= node.entries[i-1])
        {
             vector<T> child_res = range_search(start_key, end_key, node.children[i]);
             results.insert(results.end(), child_res.begin(), child_res.end());
        }
    }
    else
    {
        for (const auto& e : node.entries)
        {
            if (e >= start_key && e <= end_key)
            {
                results.push_back(e);
            }
        }
    }
    return results;
}

template <typename T>
void BTree<T>::print_tree()
{
    print_tree(root_page_id, 0);
}

template <typename T>
void BTree<T>::print_tree(int page_id, int level)
{
    if (page_id == -1) page_id = root_page_id;

    char buffer[PAGE_SIZE];
    dm->read_page(page_id, buffer);
    Node<T> node(page_id, true);
    node.deserialize(buffer);

    string indent(level * 4, ' ');

    cout << indent << "Page " << page_id << " [" << (node.is_leaf ? "LEAF" : "INTERNAL") << "] Keys: ";
    for (const auto& e : node.entries)
    {
        e.print(); 
    }
    cout << endl;

    if (!node.is_leaf)
    {
        for (int child_id : node.children)
        {
            print_tree(child_id, level + 1);
        }
    }
}