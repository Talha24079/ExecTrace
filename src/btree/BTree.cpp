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

void BTree::insert(int key) 
{
    char buffer[PAGE_SIZE];
    dm->read_page(root_page_id, buffer);
    Node root(root_page_id, false);
    root.deserialize(buffer);
    
    if (root.keys.size() == 2 * DEGREE - 1) 
    {
        int old_root_new_id = dm->allocate_page();
        root.page_id = old_root_new_id;
        char child_buf[PAGE_SIZE];
        root.serialize(child_buf);
        dm->write_page(old_root_new_id, child_buf);
        Node new_root(root_page_id, false);
        new_root.children.push_back(old_root_new_id);

        char root_buf[PAGE_SIZE];
        new_root.serialize(root_buf);
        dm->write_page(root_page_id, root_buf);

        split_child(root_page_id, 0, old_root_new_id);

        insert_non_full(root_page_id, key);
    } 
    else 
    {
        insert_non_full(root_page_id, key);
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
    Node child(child_id, false); 
    child.deserialize(c_buf);

    int sibling_id = dm->allocate_page();
    Node sibling(sibling_id, child.is_leaf);

    for (int j = 0; j < DEGREE - 1; j++) 
    {
        sibling.keys.push_back(child.keys[DEGREE + j]);
    }

    if (!child.is_leaf) 
    {
        for (int j = 0; j < DEGREE; j++) 
        {
            sibling.children.push_back(child.children[DEGREE + j]);
        }
    }

    int median_key = child.keys[DEGREE - 1];

    child.keys.resize(DEGREE - 1);
    if (!child.is_leaf) 
        child.children.resize(DEGREE);

    parent.children.insert(parent.children.begin() + index + 1, sibling_id);
    parent.keys.insert(parent.keys.begin() + index, median_key);

    char buf[PAGE_SIZE];
    
    child.serialize(buf);
    dm->write_page(child_id, buf);

    sibling.serialize(buf);
    dm->write_page(sibling_id, buf);

    parent.serialize(buf);
    dm->write_page(parent_id, buf);
}

void BTree::insert_non_full(int page_id, int key) 
{
    char buffer[PAGE_SIZE];
    dm->read_page(page_id, buffer);
    Node node(page_id, false);
    node.deserialize(buffer);

    if (node.is_leaf)
    {
        int i = node.keys.size() - 1;
        node.keys.push_back(0);

        while (i >= 0 && key < node.keys[i])
        {
            node.keys[i + 1] = node.keys[i];
            i--;
        }

        node.keys[i + 1] = key;
        node.serialize(buffer);
        dm->write_page(page_id, buffer);
    }
    else
    {
        int i = node.keys.size() - 1;
        while (i >= 0 && key < node.keys[i])
            i--;
        i++;

        int child_id = node.children[i];

        char child_buf[PAGE_SIZE];
        dm->read_page(child_id, child_buf);
        Node child(child_id, false);
        child.deserialize(child_buf);

        if (child.keys.size() == 2 * DEGREE - 1)
        {
            split_child(page_id, i, child_id);
            dm->read_page(page_id, buffer);
            node.deserialize(buffer);
            if (key > node.keys[i])
                i++;
        }

        insert_non_full(node.children[i], key);
    }
}

void BTree::print_tree(int page_id, int level) 
{
    if (page_id == -1) 
        page_id = root_page_id;

    char buffer[PAGE_SIZE];
    dm->read_page(page_id, buffer);
    Node node(page_id, false);
    node.deserialize(buffer);

    for (int i = 0; i < level; i++) 
        cout << "  ";

    cout << "Page " << page_id << ": ";

    for (int k : node.keys) 
        cout << k << " ";
    cout << endl;

    if (!node.is_leaf) 
    {
        for (int child : node.children) 
        {
            print_tree(child, level + 1);
        }
    }
}

bool BTree::search(int key, int page_id) 
{
    if (page_id == -1) 
        page_id = root_page_id;

    char buffer[PAGE_SIZE];
    dm->read_page(page_id, buffer);
    Node node(page_id, true);
    node.deserialize(buffer);

    int i = 0;
    while (i < node.keys.size() && key > node.keys[i]) 
    {
        i++;
    }

    if (i < node.keys.size() && key == node.keys[i]) 
        return true;

    if (node.is_leaf) 
        return false;

    return search(key, node.children[i]);
}

vector<int> BTree::range_search(int start_key, int end_key, int page_id)
 {
    if (page_id == -1) 
    page_id = root_page_id;

    vector<int> results;
    char buffer[PAGE_SIZE];
    dm->read_page(page_id, buffer);
    Node node(page_id, true);
    node.deserialize(buffer);

    int i = 0;
    
    if (!node.is_leaf) 
    {
        for (i = 0; i < node.keys.size(); i++) 
        {
            if (start_key <= node.keys[i]) 
            {
                vector<int> child_res = range_search(start_key, end_key, node.children[i]);
                results.insert(results.end(), child_res.begin(), child_res.end());
            }
            
            if (node.keys[i] >= start_key && node.keys[i] <= end_key) 
                results.push_back(node.keys[i]);
        }

        if (end_key >= node.keys[i-1]) 
        {
             vector<int> child_res = range_search(start_key, end_key, node.children[i]);
             results.insert(results.end(), child_res.begin(), child_res.end());
        }
    } 
    else 
    {
        
        for (int k : node.keys) 
        {
            if (k >= start_key && k <= end_key)
                results.push_back(k);
        }
    }
    return results;
}