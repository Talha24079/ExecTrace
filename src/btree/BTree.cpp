#include "../../include/btree/BTree.hpp"

BTree::BTree(DiskManager* disk_manager) : dm(disk_manager)
{
    root_page_id = 0;
    
    char buffer[PAGE_SIZE];
    dm->read_page(0, buffer);
    
    bool is_empty = true;
    for(int i=0; i<PAGE_SIZE; i++)
    {
        if(buffer[i] != 0)
        {
            is_empty = false;
            break;
        }
    }

    if (is_empty)
    {
        Node root(root_page_id, true);
        root.serialize(buffer);
        dm->write_page(root_page_id, buffer);
    }
}

void BTree::insert(TraceEntry entry)
{
    char buffer[PAGE_SIZE];
    dm->read_page(root_page_id, buffer);
    Node root(root_page_id, true);
    root.deserialize(buffer);

    if (root.entries.size() == 2 * DEGREE - 1)
    {
        int new_root_id = dm->allocate_page();
        Node new_root(new_root_id, false);
        
        new_root.children.push_back(root_page_id);
        
        Node old_root_moved = root; 
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

void BTree::split_child(int parent_id, int index, int child_id)
{
    char p_buf[PAGE_SIZE];
    dm->read_page(parent_id, p_buf);
    Node parent(parent_id, false);
    parent.deserialize(p_buf);

    char c_buf[PAGE_SIZE];
    dm->read_page(child_id, c_buf);
    Node child(child_id, true); 
    child.deserialize(c_buf);
    
    if (!child.children.empty()) child.is_leaf = false;

    int sibling_id = dm->allocate_page();
    Node sibling(sibling_id, child.is_leaf);

    for (int j = 0; j < DEGREE - 1; j++)
    {
        sibling.entries.push_back(child.entries[DEGREE + j]);
    }

    if (!child.is_leaf)
    {
        for (int j = 0; j < DEGREE; j++)
        {
            sibling.children.push_back(child.children[DEGREE + j]);
        }
    }

    TraceEntry median_entry = child.entries[DEGREE - 1];

    child.entries.resize(DEGREE - 1);
    if (!child.is_leaf)
        child.children.resize(DEGREE);

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

void BTree::insert_non_full(int page_id, TraceEntry entry)
{
    char buffer[PAGE_SIZE];
    dm->read_page(page_id, buffer);
    Node node(page_id, true); 
    node.deserialize(buffer);

    if (!node.children.empty()) node.is_leaf = false;

    int i = node.entries.size() - 1;

    if (node.is_leaf)
    {
        node.entries.push_back(TraceEntry()); 
        while (i >= 0 && entry.id < node.entries[i].id)
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
        while (i >= 0 && entry.id < node.entries[i].id)
            i--;

        i++;
        int child_id = node.children[i];
        
        char child_buf[PAGE_SIZE];
        dm->read_page(child_id, child_buf);
        Node child(child_id, true);
        child.deserialize(child_buf);

        if (child.entries.size() == 2 * DEGREE - 1)
        {
            split_child(page_id, i, child_id);
            
            dm->read_page(page_id, buffer);
            node.entries.clear();
            node.children.clear();
            node.deserialize(buffer);
            if (!node.children.empty()) node.is_leaf = false;
            
            if (entry.id > node.entries[i].id)
                i++;
        }
        insert_non_full(node.children[i], entry);
    }
}

void BTree::remove(int key)
{
    if (root_page_id == -1) return;

    _remove(root_page_id, key);

    char buffer[PAGE_SIZE];
    dm->read_page(root_page_id, buffer);
    Node root(root_page_id, true);
    root.deserialize(buffer);

    if (root.entries.empty())
    {
        if (root.is_leaf)
        {
            root_page_id = -1;
        }
        else
        {
            root_page_id = root.children[0];
        }
    }
}

void BTree::_remove(int page_id, int key)
{
    char buffer[PAGE_SIZE];
    dm->read_page(page_id, buffer);
    Node node(page_id, true);
    node.deserialize(buffer);

    int idx = 0;
    while (idx < node.entries.size() && node.entries[idx].id < key)
    {
        idx++;
    }

    if (idx < node.entries.size() && node.entries[idx].id == key)
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
            Node left_child(left_child_id, true); 
            left_child.deserialize(left_buf);

            if (left_child.entries.size() >= DEGREE)
            {
                TraceEntry pred = get_predecessor(node.children[idx]);
                node.entries[idx] = pred;
                node.serialize(buffer);
                dm->write_page(page_id, buffer);
                _remove(node.children[idx], pred.id);
            }
            else
            {
                int right_child_id = node.children[idx + 1];
                char right_buf[PAGE_SIZE];
                dm->read_page(right_child_id, right_buf);
                Node right_child(right_child_id, true); 
                right_child.deserialize(right_buf);

                if (right_child.entries.size() >= DEGREE)
                {
                    TraceEntry succ = get_successor(node.children[idx + 1]);
                    node.entries[idx] = succ;
                    node.serialize(buffer);
                    dm->write_page(page_id, buffer);
                    _remove(node.children[idx + 1], succ.id);
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
        Node child_check(child_id, true); 
        child_check.deserialize(child_buf);

        if (child_check.entries.size() < DEGREE)
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

TraceEntry BTree::get_predecessor(int page_id)
{
    char buf[PAGE_SIZE];
    dm->read_page(page_id, buf);
    Node curr(page_id, true); 
    curr.deserialize(buf);

    while (!curr.is_leaf)
    {
        int next_page = curr.children.back();
        dm->read_page(next_page, buf);
        curr = Node(next_page, true);
        curr.deserialize(buf);
    }
    return curr.entries.back();
}

TraceEntry BTree::get_successor(int page_id)
{
    char buf[PAGE_SIZE];
    dm->read_page(page_id, buf);
    Node curr(page_id, true); 
    curr.deserialize(buf);

    while (!curr.is_leaf)
    {
        int next_page = curr.children.front();
        dm->read_page(next_page, buf);
        curr = Node(next_page, true);
        curr.deserialize(buf);
    }
    return curr.entries.front();
}

void BTree::fill(int page_id, int idx)
{
    char p_buf[PAGE_SIZE];
    dm->read_page(page_id, p_buf);
    Node parent(page_id, false);
    parent.deserialize(p_buf);

    if (idx != 0)
    {
        int sibling_id = parent.children[idx - 1];
        char s_buf[PAGE_SIZE];
        dm->read_page(sibling_id, s_buf);
        Node sibling(sibling_id, true); 
        sibling.deserialize(s_buf);
        
        if (sibling.entries.size() >= DEGREE)
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
        Node sibling(sibling_id, true); 
        sibling.deserialize(s_buf);

        if (sibling.entries.size() >= DEGREE)
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

void BTree::borrow_from_prev(int page_id, int idx)
{
    char p_buf[PAGE_SIZE];
    dm->read_page(page_id, p_buf);
    Node parent(page_id, false);
    parent.deserialize(p_buf);

    int child_id = parent.children[idx];
    int sibling_id = parent.children[idx - 1];

    char c_buf[PAGE_SIZE], s_buf[PAGE_SIZE];
    dm->read_page(child_id, c_buf);
    dm->read_page(sibling_id, s_buf);
    Node child(child_id, true); child.deserialize(c_buf);
    Node sibling(sibling_id, true); sibling.deserialize(s_buf);

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

void BTree::borrow_from_next(int page_id, int idx)
{
    char p_buf[PAGE_SIZE];
    dm->read_page(page_id, p_buf);
    Node parent(page_id, false);
    parent.deserialize(p_buf);

    int child_id = parent.children[idx];
    int sibling_id = parent.children[idx + 1];

    char c_buf[PAGE_SIZE], s_buf[PAGE_SIZE];
    dm->read_page(child_id, c_buf);
    dm->read_page(sibling_id, s_buf);
    Node child(child_id, true); child.deserialize(c_buf);
    Node sibling(sibling_id, true); sibling.deserialize(s_buf);

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

void BTree::merge(int page_id, int idx)
{
    char p_buf[PAGE_SIZE];
    dm->read_page(page_id, p_buf);
    Node parent(page_id, false);
    parent.deserialize(p_buf);

    int child_id = parent.children[idx];
    int sibling_id = parent.children[idx + 1];

    char c_buf[PAGE_SIZE], s_buf[PAGE_SIZE];
    dm->read_page(child_id, c_buf);
    dm->read_page(sibling_id, s_buf);
    Node child(child_id, true); child.deserialize(c_buf);
    Node sibling(sibling_id, true); sibling.deserialize(s_buf);

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

bool BTree::search(int key, int page_id)
{
    if (page_id == -1) page_id = root_page_id;

    char buffer[PAGE_SIZE];
    dm->read_page(page_id, buffer);
    Node node(page_id, true);
    node.deserialize(buffer);

    int i = 0;
    while (i < node.entries.size() && key > node.entries[i].id)
    {
        i++;
    }

    if (i < node.entries.size() && key == node.entries[i].id)
    {
        return true;
    }

    if (node.is_leaf)
    {
        return false;
    }

    return search(key, node.children[i]);
}

vector<TraceEntry> BTree::range_search(int start_key, int end_key, int page_id)
{
    if (page_id == -1) page_id = root_page_id;

    vector<TraceEntry> results;
    char buffer[PAGE_SIZE];
    dm->read_page(page_id, buffer);
    Node node(page_id, true);
    node.deserialize(buffer);

    int i = 0;
    
    if (!node.is_leaf)
    {
        for (i = 0; i < node.entries.size(); i++)
        {
            if (start_key <= node.entries[i].id)
            {
                vector<TraceEntry> child_res = range_search(start_key, end_key, node.children[i]);
                results.insert(results.end(), child_res.begin(), child_res.end());
            }
            
            if (node.entries[i].id >= start_key && node.entries[i].id <= end_key)
            {
                results.push_back(node.entries[i]);
            }
        }
        if (end_key >= node.entries[i-1].id)
        {
             vector<TraceEntry> child_res = range_search(start_key, end_key, node.children[i]);
             results.insert(results.end(), child_res.begin(), child_res.end());
        }
    }
    else
    {
        for (const auto& e : node.entries)
        {
            if (e.id >= start_key && e.id <= end_key)
            {
                results.push_back(e);
            }
        }
    }
    return results;
}

void BTree::print_tree(int page_id, int level)
{
    if (page_id == -1) page_id = root_page_id;

    char buffer[PAGE_SIZE];
    dm->read_page(page_id, buffer);
    Node node(page_id, true);
    node.deserialize(buffer);

    string indent(level * 4, ' ');

    cout << indent << "Page " << page_id << " [" << (node.is_leaf ? "LEAF" : "INTERNAL") << "] Keys: ";
    for (const auto& e : node.entries)
    {
        cout << e.id << " ";
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