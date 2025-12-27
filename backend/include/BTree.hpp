#pragma once
#include "DiskManager.hpp"
#include "Models.hpp"
#include <vector>
#include <algorithm>

using namespace ExecTrace;

template <typename T>
class Node {
public:
    int page_id;
    bool is_leaf;
    std::vector<T> entries;
    std::vector<int> children;

    static const int CAPACITY = (PAGE_SIZE - sizeof(bool) - 2 * sizeof(int)) / (sizeof(T) + sizeof(int));
    static const int DEGREE = (CAPACITY + 1) / 2;
    static const int MAX_KEYS = 2 * DEGREE - 1;

    Node(int id, bool leaf) : page_id(id), is_leaf(leaf) {}
    Node() : page_id(0), is_leaf(true) {}

    void serialize(char* buffer) const {
        memset(buffer, 0, PAGE_SIZE);
        int offset = 0;

        memcpy(buffer + offset, &is_leaf, sizeof(bool));
        offset += sizeof(bool);

        int count = entries.size();
        memcpy(buffer + offset, &count, sizeof(int));
        offset += sizeof(int);

        if (count > 0) {
            memcpy(buffer + offset, entries.data(), count * sizeof(T));
            offset += count * sizeof(T);
        }

        int child_count = children.size();
        memcpy(buffer + offset, &child_count, sizeof(int));
        offset += sizeof(int);

        if (child_count > 0) {
            memcpy(buffer + offset, children.data(), child_count * sizeof(int));
        }
    }

    void deserialize(char* buffer) {
        int offset = 0;

        memcpy(&is_leaf, buffer + offset, sizeof(bool));
        offset += sizeof(bool);

        int count;
        memcpy(&count, buffer + offset, sizeof(int));
        offset += sizeof(int);

        if (count < 0 || count > MAX_KEYS) {
            count = 0;
        }

        entries.clear();
        if (count > 0) {
            entries.resize(count);
            size_t entry_bytes = count * sizeof(T);
            if (offset + entry_bytes <= PAGE_SIZE) {
                memcpy(entries.data(), buffer + offset, entry_bytes);
                offset += entry_bytes;
            } else {
                entries.clear();
            }
        }

        int child_count;
        memcpy(&child_count, buffer + offset, sizeof(int));
        offset += sizeof(int);

        if (child_count < 0 || child_count > MAX_KEYS + 1) {
            child_count = 0;
        }

        children.clear();
        if (child_count > 0 && !is_leaf) {
            children.resize(child_count);
            size_t child_bytes = child_count * sizeof(int);
            if (offset + child_bytes <= PAGE_SIZE) {
                memcpy(children.data(), buffer + offset, child_bytes);
            } else {
                children.clear();
            }
        }

        if (!is_leaf && children.empty()) {

            is_leaf = true;
        }
    }
};

template <typename T>
class BTree {
private:
    DiskManager* dm;
    int root_page_id;

public:
    BTree(DiskManager* disk_manager) : dm(disk_manager), root_page_id(0) {
        char buffer[PAGE_SIZE];
        dm->read_page(0, buffer);

        bool is_empty = true;
        for(int i = 0; i < 10; i++) {
            if(buffer[i] != 0) {
                is_empty = false;
                break;
            }
        }

        if (is_empty) {
            
            Node<T> root(0, true);
            root.serialize(buffer);
            dm->write_page(0, buffer);
            std::cout << "[BTree] Initialized new root page" << std::endl;
        } else {
            std::cout << "[BTree] Loaded existing root page" << std::endl;
        }
    }

    void insert(const T& entry) {
        Node<T> root = load_node(root_page_id);
        
        if (root.entries.size() == Node<T>::MAX_KEYS) {
            
            int new_root_id = dm->allocate_page();
            Node<T> new_root(new_root_id, false);
            new_root.children.push_back(root_page_id);
            
            split_child(new_root, 0);
            root_page_id = new_root_id;
            save_node(new_root);
            
            insert_non_full(new_root, entry);
        } else {
            insert_non_full(root, entry);
        }
    }

    std::vector<T> search(const T& key) {
        return search_node(root_page_id, key);
    }

private:
    Node<T> load_node(int page_id) {
        char buffer[PAGE_SIZE];
        dm->read_page(page_id, buffer);
        Node<T> node(page_id, true);
        node.deserialize(buffer);
        return node;
    }

    void save_node(const Node<T>& node) {
        char buffer[PAGE_SIZE];
        node.serialize(buffer);
        dm->write_page(node.page_id, buffer);
    }

    void insert_non_full(Node<T>& node, const T& entry) {
        // CHECK 1: If entry exists in this node, update it and return
        for (auto& e : node.entries) {
            if (e == entry) { // Relies on operator== checking the ID
                e = entry;    // Update the existing entry
                save_node(node);
                std::cout << "[BTree] Updated existing entry in node " << node.page_id << std::endl;
                return;
            }
        }

        if (node.is_leaf) {
            // Standard insertion into leaf
            node.entries.push_back(entry);
            std::sort(node.entries.begin(), node.entries.end());
            save_node(node);
        } else {
            // Find child to recurse into
            int i = node.entries.size() - 1;
            while (i >= 0 && entry < node.entries[i]) {
                i--;
            }
            i++;

            Node<T> child = load_node(node.children[i]);
            
            if (child.entries.size() == Node<T>::MAX_KEYS) {
                split_child(node, i);
                if (entry > node.entries[i]) {
                    i++;
                }
                child = load_node(node.children[i]);
            }
            
            insert_non_full(child, entry);
        }
    }

    void split_child(Node<T>& parent, int index) {
        Node<T> child = load_node(parent.children[index]);
        int new_page_id = dm->allocate_page();
        Node<T> new_node(new_page_id, child.is_leaf);

        int mid = Node<T>::DEGREE - 1;

        new_node.entries.insert(new_node.entries.begin(), 
                                child.entries.begin() + mid + 1, 
                                child.entries.end());
        
        if (!child.is_leaf) {
            new_node.children.insert(new_node.children.begin(),
                                     child.children.begin() + mid + 1,
                                     child.children.end());
            child.children.erase(child.children.begin() + mid + 1, child.children.end());
        }
        
        T mid_entry = child.entries[mid];
        child.entries.erase(child.entries.begin() + mid, child.entries.end());

        parent.entries.insert(parent.entries.begin() + index, mid_entry);
        parent.children.insert(parent.children.begin() + index + 1, new_page_id);
        
        save_node(child);
        save_node(new_node);
        save_node(parent);
    }

    std::vector<T> search_node(int page_id, const T& key) {
        Node<T> node = load_node(page_id);
        std::vector<T> results;
        
        for (const auto& entry : node.entries) {
            if (entry == key) {
                results.push_back(entry);
            }
        }
        
        if (!node.is_leaf) {
            for (int child_id : node.children) {
                auto child_results = search_node(child_id, key);
                results.insert(results.end(), child_results.begin(), child_results.end());
            }
        }
        
        return results;
    }
    
public:
    
    std::vector<T> get_all_values() {
        std::vector<T> result;
        collect_all_values(root_page_id, result);
        return result;
    }

private:
    
    void collect_all_values(int page_id, std::vector<T>& result) {
        Node<T> node = load_node(page_id);
        
        if (node.is_leaf) {
            
            for (const auto& entry : node.entries) {
                result.push_back(entry);
            }
        } else {
            
            for (size_t i = 0; i < node.entries.size(); i++) {
                
                if (i < node.children.size()) {
                    collect_all_values(node.children[i], result);
                }
                
                result.push_back(node.entries[i]);
            }
            
            if (!node.children.empty() && node.children.size() > node.entries.size()) {
                collect_all_values(node.children[node.children.size() - 1], result);
            }
        }
    }
};
