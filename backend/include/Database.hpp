#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <cstring>
#include <mutex>
#include <random>
#include <map>
#include "Models.hpp"

using namespace std;
using namespace ExecTrace;

const int INVALID_PAGE = -1;
const int PAGE_SIZE = 4096;

class DiskManager {
private:
    fstream db_file;
    string file_name;
    int next_page_id;
    mutable mutex file_mutex;

public:
    DiskManager(string filename) : file_name(filename), next_page_id(1) {
        db_file.open(file_name, ios::in | ios::out | ios::binary);
        
        if (!db_file.is_open()) {
            db_file.open(file_name, ios::out | ios::binary);
            db_file.close();
            db_file.open(file_name, ios::in | ios::out | ios::binary);
        }
        
        db_file.seekg(0, ios::end);
        auto file_size = db_file.tellg();
        next_page_id = (file_size / PAGE_SIZE) +1;
    }

    ~DiskManager() {
        if (db_file.is_open()) {
            db_file.close();
        }
    }

    void write_page(int page_id, const char* data) {
        lock_guard<mutex> lock(file_mutex);
        db_file.seekp(page_id * PAGE_SIZE, ios::beg);
        db_file.write(data, PAGE_SIZE);
        db_file.flush();
    }

    void read_page(int page_id, char* data) {
        lock_guard<mutex> lock(file_mutex);
        db_file.seekg(page_id * PAGE_SIZE, ios::beg);
        db_file.read(data, PAGE_SIZE);
    }

    int allocate_page() {
        lock_guard<mutex> lock(file_mutex);
        return next_page_id++;
    }
};

template <typename T>
class Node {
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

    void serialize(char* buffer) {
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

        if (!is_leaf) {
            memcpy(buffer + offset, children.data(), children.size() * sizeof(int));
        }
    }

    void deserialize(char* buffer) {
        int offset = 0;

        memcpy(&is_leaf, buffer + offset, sizeof(bool));
        offset += sizeof(bool);

        int count;
        memcpy(&count, buffer + offset, sizeof(int));
        offset += sizeof(int);

        entries.resize(count);
        if (count > 0) {
            memcpy(entries.data(), buffer + offset, count * sizeof(T));
            offset += count * sizeof(T);
        }

        if (!is_leaf) {
            children.resize(count + 1);
            memcpy(children.data(), buffer + offset, (count + 1) * sizeof(int));
        }
    }
};

template <typename T>
class BTree {
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
    bool _search(T key, int page_id);
    vector<T> _range_search(T start_key, T end_key, int page_id);

public:
    BTree(DiskManager* disk_manager);
    void insert(T entry);
    void remove(T key);
    bool search(T key);
    vector<T> range_search(T start_key, T end_key);
};

template <typename T>
BTree<T>::BTree(DiskManager* disk_manager) : dm(disk_manager), root_page_id(0) {
    char buffer[PAGE_SIZE];
    dm->read_page(0, buffer);
    
    bool is_empty = true;
    for(int i = 0; i < PAGE_SIZE; i++) {
        if(buffer[i] != 0) {
            is_empty = false;
            break;
        }
    }

    if (is_empty) {
        Node<T> root(root_page_id, true);
        root.serialize(buffer);
        dm->write_page(root_page_id, buffer);
    }
}

template <typename T>
void BTree<T>::insert(T entry) {
    char buffer[PAGE_SIZE];
    dm->read_page(root_page_id, buffer);
    Node<T> root(root_page_id, true);
    root.deserialize(buffer);

    if (root.entries.size() == Node<T>::MAX_KEYS) {
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
    } else {
        insert_non_full(root_page_id, entry);
    }
}

template <typename T>
void BTree<T>::split_child(int parent_id, int index, int child_id) {
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

    for (int j = 0; j < degree - 1; j++) {
        sibling.entries.push_back(child.entries[degree + j]);
    }

    if (!child.is_leaf) {
        for (int j = 0; j < degree; j++) {
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
void BTree<T>::insert_non_full(int page_id, T entry) {
    char buffer[PAGE_SIZE];
    dm->read_page(page_id, buffer);
    Node<T> node(page_id, true);
    node.deserialize(buffer);

    if (!node.children.empty()) node.is_leaf = false;

    int i = node.entries.size() - 1;

    if (node.is_leaf) {
        node.entries.push_back(T());
        while (i >= 0 && entry < node.entries[i]) {
            node.entries[i + 1] = node.entries[i];
            i--;
        }
        node.entries[i + 1] = entry;
        
        node.serialize(buffer);
        dm->write_page(page_id, buffer);
    } else {
        while (i >= 0 && entry < node.entries[i])
            i--;

        i++;
        int child_id = node.children[i];
        
        char child_buf[PAGE_SIZE];
        dm->read_page(child_id, child_buf);
        Node<T> child(child_id, true);
        child.deserialize(child_buf);

        if (child.entries.size() == Node<T>::MAX_KEYS) {
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
void BTree<T>::remove(T key) {
    if (root_page_id == -1) return;

    _remove(root_page_id, key);

    char buffer[PAGE_SIZE];
    dm->read_page(root_page_id, buffer);
    Node<T> root(root_page_id, true);
    root.deserialize(buffer);

    if (root.entries.empty()) {
        if (root.is_leaf) {
            root_page_id = 0;
            Node<T> empty_root(0, true);
            empty_root.serialize(buffer);
            dm->write_page(0, buffer);
        } else {
            root_page_id = root.children[0];
        }
    }
}

template <typename T>
void BTree<T>::_remove(int page_id, T key) {
    char buffer[PAGE_SIZE];
    dm->read_page(page_id, buffer);
    Node<T> node(page_id, true);
    node.deserialize(buffer);

    int degree = Node<T>::DEGREE;

    int idx = 0;
    while (idx < node.entries.size() && node.entries[idx] < key) {
        idx++;
    }

    if (idx < node.entries.size() && node.entries[idx] == key) {
        if (node.is_leaf) {
            node.entries.erase(node.entries.begin() + idx);
            node.serialize(buffer);
            dm->write_page(page_id, buffer);
        } else {
            int left_child_id = node.children[idx];
            char left_buf[PAGE_SIZE];
            dm->read_page(left_child_id, left_buf);
            Node<T> left_child(left_child_id, true);
            left_child.deserialize(left_buf);

            if (left_child.entries.size() >= degree) {
                T pred = get_predecessor(node.children[idx]);
                node.entries[idx] = pred;
                node.serialize(buffer);
                dm->write_page(page_id, buffer);
                _remove(node.children[idx], pred);
            } else {
                int right_child_id = node.children[idx + 1];
                char right_buf[PAGE_SIZE];
                dm->read_page(right_child_id, right_buf);
                Node<T> right_child(right_child_id, true);
                right_child.deserialize(right_buf);

                if (right_child.entries.size() >= degree) {
                    T succ = get_successor(node.children[idx + 1]);
                    node.entries[idx] = succ;
                    node.serialize(buffer);
                    dm->write_page(page_id, buffer);
                    _remove(node.children[idx + 1], succ);
                } else {
                    merge(page_id, idx);
                    _remove(node.children[idx], key);
                }
            }
        }
    } else {
        if (node.is_leaf) return;

        bool flag = (idx == node.entries.size());

        int child_id = node.children[idx];
        char child_buf[PAGE_SIZE];
        dm->read_page(child_id, child_buf);
        Node<T> child_check(child_id, true);
        child_check.deserialize(child_buf);

        if (child_check.entries.size() < degree) {
            fill(page_id, idx);
            dm->read_page(page_id, buffer);
            node.entries.clear();
            node.children.clear();
            node.deserialize(buffer);
        }

        if (flag && idx > node.entries.size()) {
            _remove(node.children[idx - 1], key);
        } else {
            _remove(node.children[idx], key);
        }
    }
}

template <typename T>
T BTree<T>::get_predecessor(int page_id) {
    char buf[PAGE_SIZE];
    dm->read_page(page_id, buf);
    Node<T> curr(page_id, true);
    curr.deserialize(buf);

    while (!curr.is_leaf) {
        int next_page = curr.children.back();
        dm->read_page(next_page, buf);
        curr = Node<T>(next_page, true);
        curr.deserialize(buf);
    }
    return curr.entries.back();
}

template <typename T>
T BTree<T>::get_successor(int page_id) {
    char buf[PAGE_SIZE];
    dm->read_page(page_id, buf);
    Node<T> curr(page_id, true);
    curr.deserialize(buf);

    while (!curr.is_leaf) {
        int next_page = curr.children.front();
        dm->read_page(next_page, buf);
        curr = Node<T>(next_page, true);
        curr.deserialize(buf);
    }
    return curr.entries.front();
}

template <typename T>
void BTree<T>::fill(int page_id, int idx) {
    int degree = Node<T>::DEGREE;

    char p_buf[PAGE_SIZE];
    dm->read_page(page_id, p_buf);
    Node<T> parent(page_id, false);
    parent.deserialize(p_buf);

    if (idx != 0) {
        int sibling_id = parent.children[idx - 1];
        char s_buf[PAGE_SIZE];
        dm->read_page(sibling_id, s_buf);
        Node<T> sibling(sibling_id, true);
        sibling.deserialize(s_buf);
        
        if (sibling.entries.size() >= degree) {
            borrow_from_prev(page_id, idx);
            return;
        }
    }

    if (idx != parent.entries.size()) {
        int sibling_id = parent.children[idx + 1];
        char s_buf[PAGE_SIZE];
        dm->read_page(sibling_id, s_buf);
        Node<T> sibling(sibling_id, true);
        sibling.deserialize(s_buf);

        if (sibling.entries.size() >= degree) {
            borrow_from_next(page_id, idx);
            return;
        }
    }

    if (idx != parent.entries.size()) {
        merge(page_id, idx);
    } else {
        merge(page_id, idx - 1);
    }
}

template <typename T>
void BTree<T>::borrow_from_prev(int page_id, int idx) {
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
    
    if (!child.is_leaf) {
        child.children.insert(child.children.begin(), sibling.children.back());
    }

    parent.entries[idx - 1] = sibling.entries.back();

    sibling.entries.pop_back();
    if (!sibling.is_leaf) {
        sibling.children.pop_back();
    }

    char buf[PAGE_SIZE];
    parent.serialize(buf); dm->write_page(page_id, buf);
    child.serialize(buf); dm->write_page(child_id, buf);
    sibling.serialize(buf); dm->write_page(sibling_id, buf);
}

template <typename T>
void BTree<T>::borrow_from_next(int page_id, int idx) {
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

    if (!child.is_leaf) {
        child.children.push_back(sibling.children[0]);
    }

    parent.entries[idx] = sibling.entries[0];

    sibling.entries.erase(sibling.entries.begin());
    if (!sibling.is_leaf) {
        sibling.children.erase(sibling.children.begin());
    }

    char buf[PAGE_SIZE];
    parent.serialize(buf); dm->write_page(page_id, buf);
    child.serialize(buf); dm->write_page(child_id, buf);
    sibling.serialize(buf); dm->write_page(sibling_id, buf);
}

template <typename T>
void BTree<T>::merge(int page_id, int idx) {
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

    if (!child.is_leaf) {
        for (int c : sibling.children) child.children.push_back(c);
    }

    parent.entries.erase(parent.entries.begin() + idx);
    parent.children.erase(parent.children.begin() + idx + 1);

    char buf[PAGE_SIZE];
    parent.serialize(buf); dm->write_page(page_id, buf);
    child.serialize(buf); dm->write_page(child_id, buf);
}

template <typename T>
bool BTree<T>::search(T key) {
    return _search(key, root_page_id);
}

template <typename T>
bool BTree<T>::_search(T key, int page_id) {
    char buffer[PAGE_SIZE];
    dm->read_page(page_id, buffer);
    Node<T> node(page_id, true);
    node.deserialize(buffer);

    int i = 0;
    while (i < node.entries.size() && key > node.entries[i]) {
        i++;
    }

    if (i < node.entries.size() && key == node.entries[i]) {
        return true;
    }

    if (node.is_leaf) {
        return false;
    }

    return _search(key, node.children[i]);
}

template <typename T>
vector<T> BTree<T>::range_search(T start_key, T end_key) {
    return _range_search(start_key, end_key, root_page_id);
}

template <typename T>
vector<T> BTree<T>::_range_search(T start_key, T end_key, int page_id) {
    vector<T> results;
    char buffer[PAGE_SIZE];
    dm->read_page(page_id, buffer);
    Node<T> node(page_id, true);
    node.deserialize(buffer);

    int i = 0;
    
    if (!node.is_leaf) {
        for (i = 0; i < node.entries.size(); i++) {
            if (start_key <= node.entries[i]) {
                vector<T> child_res = _range_search(start_key, end_key, node.children[i]);
                results.insert(results.end(), child_res.begin(), child_res.end());
            }
            
            if (node.entries[i] >= start_key && node.entries[i] <= end_key) {
                results.push_back(node.entries[i]);
            }
        }
        if (end_key >= node.entries[i-1]) {
             vector<T> child_res = _range_search(start_key, end_key, node.children[i]);
             results.insert(results.end(), child_res.begin(), child_res.end());
        }
    } else {
        for (const auto& e : node.entries) {
            if (e >= start_key && e <= end_key) {
                results.push_back(e);
            }
        }
    }
    return results;
}

class AuthDB {
private:
    DiskManager* dm_users;
    DiskManager* dm_projects;
    BTree<UserEntry>* user_tree;
    BTree<ProjectEntry>* project_tree;
    mutex auth_mutex;
    int next_user_id;
    int next_project_id;
    
    map<string, UserEntry> user_by_email;

    string generate_api_key(const string& prefix = "sk_") {
        string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        string key = prefix;

        static random_device rd;
        static mt19937 gen(rd());
        uniform_int_distribution<> dis(0, chars.length() - 1);

        for (int i = 0; i < 32; i++) {
            key += chars[dis(gen)];
        }
        return key;
    }

public:
    AuthDB(string user_file, string proj_file) {
        dm_users = new DiskManager(user_file);
        dm_projects = new DiskManager(proj_file);
        
        user_tree = new BTree<UserEntry>(dm_users);
        project_tree = new BTree<ProjectEntry>(dm_projects);
        
        next_user_id = 1;
        next_project_id = 1;
    }

    ~AuthDB() {
        delete user_tree;
        delete project_tree;
        delete dm_users;
        delete dm_projects;
    }

    bool register_user(const string& email, const string& password, const string& username, string& error) {
        lock_guard<mutex> lock(auth_mutex);
        
        if (email.empty() || password.empty() || username.empty()) {
            error = "All fields required";
            return false;
        }

        if (user_by_email.find(email) != user_by_email.end()) {
            error = "Email already registered";
            return false;
        }

        UserEntry new_user(email.c_str(), password.c_str(), username.c_str(), next_user_id++);
        user_by_email[email] = new_user;
        return true;
    }

    bool login_user(const string& email, const string& password, int& user_id, string& username_out, string& error) {
        lock_guard<mutex> lock(auth_mutex);

        auto it = user_by_email.find(email);
        if (it != user_by_email.end()) {
            if (strcmp(it->second.password, password.c_str()) == 0) {
                user_id = it->second.user_id;
                username_out = string(it->second.username);
                return true;
            }
        }

        error = "Invalid email or password";
        return false;
    }

    string create_project(int owner_id, string project_name) {
        lock_guard<mutex> lock(auth_mutex);
        
        string new_key = generate_api_key("sk_live_");
        
        ProjectEntry proj(new_key.c_str(), next_project_id++, owner_id, project_name.c_str());
        project_tree->insert(proj);
        
        return new_key;
    }

    bool validate_project_key(string api_key, ProjectEntry& out_proj) {
        lock_guard<mutex> lock(auth_mutex);
        ProjectEntry target;
        strncpy(target.api_key, api_key.c_str(), 63);

        vector<ProjectEntry> result = project_tree->range_search(target, target);
        if (!result.empty()) {
            out_proj = result[0];
            return true;
        }
        return false;
    }

    vector<ProjectEntry> get_user_projects(int user_id) {
        lock_guard<mutex> lock(auth_mutex);
        
        ProjectEntry min_p, max_p;
        memset(min_p.api_key, 0, 64);
        memset(max_p.api_key, 0xFF, 64);
        
        vector<ProjectEntry> all = project_tree->range_search(min_p, max_p);
        vector<ProjectEntry> filtered;
        
        for(const auto& p : all) {
            if(p.owner_id == user_id)
                filtered.push_back(p);
        }
        return filtered;
    }
};

class ExecTraceDB {
private:
    DiskManager* dm;
    BTree<TraceEntry>* trace_tree;
    mutex db_mutex;
    int next_id;
    
public:
    ExecTraceDB(string db_file) {
        dm = new DiskManager(db_file);
        trace_tree = new BTree<TraceEntry>(dm);
        next_id = 1;
    }

    ~ExecTraceDB() {
        delete trace_tree;
        delete dm;
    }

    int log_event(int project_id, const char* func, const char* msg, const char* app_version, uint64_t duration, uint64_t ram) {
        lock_guard<mutex> lock(db_mutex);
        
        TraceEntry entry(next_id++, project_id, func, msg, app_version, duration, ram);
        trace_tree->insert(entry);
        return entry.id;
    }

    vector<TraceEntry> query_range(int project_id, int min_id, int max_id) {
        lock_guard<mutex> lock(db_mutex);
        
        TraceEntry min_t, max_t;
        min_t.id = min_id;
        max_t.id = max_id;
        
        vector<TraceEntry> all = trace_tree->range_search(min_t, max_t);
        vector<TraceEntry> filtered;
        
        for(const auto& t : all) {
            if(t.project_id == project_id) {
                filtered.push_back(t);
            }
        }
        return filtered;
    }
};
