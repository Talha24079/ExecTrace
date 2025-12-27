#pragma once
#include "BTree.hpp"
#include "Models.hpp"
#include <random>
#include <sstream>
#include <iomanip>
#include <cstdio> // For std::remove

// SHA-256 for password hashing (simple implementation)
inline uint64_t simple_hash(const std::string& str) {
    uint64_t hash = 5381;
    for (char c : str) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

// Generate random API key
inline std::string generate_api_key() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    
    std::stringstream ss;
    ss << "sk_live_";
    ss << std::hex << std::setfill('0');
    ss << std::setw(16) << dis(gen);
    ss << std::setw(16) << dis(gen);
    
    return ss.str();
}

class AuthDB {
private:
    std::string user_db_path;
    std::string project_db_path;
    DiskManager* user_dm;
    DiskManager* project_dm;
    BTree<ExecTrace::UserEntry>* user_tree;
    BTree<ExecTrace::ProjectEntry>* project_tree;
    std::mutex auth_mutex;
    int next_user_id;
    int next_project_id;

    // Helper to rebuild user tree (fixes updates/deletes)
    void rebuild_user_tree(const std::vector<ExecTrace::UserEntry>& users) {
        // Close and delete current tree and manager
        delete user_tree;
        delete user_dm;
        
        // Delete the physical file to ensure we start empty
        std::remove(user_db_path.c_str());
        
        // Re-initialize manager and tree
        user_dm = new DiskManager(user_db_path);
        user_tree = new BTree<ExecTrace::UserEntry>(user_dm);
        
        // Re-insert all users (this writes to the file)
        for (const auto& user : users) {
            user_tree->insert(user);
        }
    }

public:
    AuthDB(const std::string& user_db_file, const std::string& project_db_file) 
        : next_user_id(1), next_project_id(1), 
          user_db_path(user_db_file), project_db_path(project_db_file) {
        
        user_dm = new DiskManager(user_db_file);
        project_dm = new DiskManager(project_db_file);
        
        user_tree = new BTree<ExecTrace::UserEntry>(user_dm);
        project_tree = new BTree<ExecTrace::ProjectEntry>(project_dm);
        
        // Initialize ID counters from existing data
        auto existing_users = user_tree->get_all_values();
        for (const auto& user : existing_users) {
            if (user.user_id >= next_user_id) {
                next_user_id = user.user_id + 1;
            }
        }
        
        auto existing_projects = project_tree->get_all_values();
        for (const auto& project : existing_projects) {
            if (project.project_id >= next_project_id) {
                next_project_id = project.project_id + 1;
            }
        }
        
        std::cout << "[AuthDB] Initialized authentication database" << std::endl;
        std::cout << "[AuthDB] Next user ID: " << next_user_id << ", Next project ID: " << next_project_id << std::endl;
        std::cout << "[AuthDB] Found " << existing_users.size() << " users, " << existing_projects.size() << " projects" << std::endl;
    }

    ~AuthDB() {
        delete user_tree;
        delete project_tree;
        delete user_dm;
        delete project_dm;
    }

    // Register new user
    bool register_user(const std::string& email, const std::string& password, 
                      const std::string& username, int& out_user_id) {
        std::lock_guard<std::mutex> lock(auth_mutex);
        
        // Check if user already exists
        uint64_t email_hash = simple_hash(email);
        ExecTrace::UserEntry search_key;
        search_key.email_hash = email_hash;
        
        auto results = user_tree->search(search_key);
        if (!results.empty()) {
            std::cout << "[AuthDB] User already exists: " << email << std::endl;
            return false;
        }
        
        // Create new user
        ExecTrace::UserEntry user;
        user.user_id = next_user_id++;
        user.email_hash = email_hash;
        strncpy(user.email, email.c_str(), sizeof(user.email) - 1);
        strncpy(user.username, username.c_str(), sizeof(user.username) - 1);
        
        // ROLE ASSIGNMENT: First user = Admin, others = User
        if (user.user_id == 1) {
            user.role = ExecTrace::ROLE_ADMIN;
            std::cout << "[AuthDB] First user - assigning Admin role" << std::endl;
        } else {
            user.role = ExecTrace::ROLE_USER;
        }
        
        user.is_active = true;
        user.created_at = time(nullptr);
        
        // Hash password (simple hash for demo)
        uint64_t pwd_hash = simple_hash(password);
        snprintf(user.password_hash, sizeof(user.password_hash), "%016lx", pwd_hash);
        
        user_tree->insert(user);
        out_user_id = user.user_id;
        
        const char* role_name = (user.role == ExecTrace::ROLE_ADMIN) ? "Admin" : 
                                (user.role == ExecTrace::ROLE_EDITOR) ? "Editor" : "User";
        std::cout << "[AuthDB] Registered user: " << username 
                  << " (ID: " << user.user_id << ", Role: " << role_name << ")" << std::endl;
        return true;
    }

    // Login user
    bool login_user(const std::string& email, const std::string& password, 
                   ExecTrace::UserEntry& out_user) {
        std::lock_guard<std::mutex> lock(auth_mutex);
        
        uint64_t email_hash = simple_hash(email);
        ExecTrace::UserEntry search_key;
        search_key.email_hash = email_hash;
        
        auto results = user_tree->search(search_key);
        if (results.empty()) {
            std::cout << "[AuthDB] Login failed: user not found" << std::endl;
            return false;
        }
        
        // Verify password
        uint64_t pwd_hash = simple_hash(password);
        char pwd_hash_str[65];
        snprintf(pwd_hash_str, sizeof(pwd_hash_str), "%016lx", pwd_hash);
        
        if (strcmp(results[0].password_hash, pwd_hash_str) != 0) {
            std::cout << "[AuthDB] Login failed: incorrect password" << std::endl;
            return false;
        }
        
        // Check if user is active
        if (!results[0].is_active) {
            std::cout << "[AuthDB] Login failed: user account is deactivated" << std::endl;
            return false;
        }
        
        out_user = results[0];
        std::cout << "[AuthDB] Login successful: " << out_user.username 
                  << " (Role: " << out_user.role << ")" << std::endl;
        return true;
    }

    // Create project
    bool create_project(int user_id, const std::string& project_name, 
                       std::string& out_api_key, int& out_project_id) {
        std::lock_guard<std::mutex> lock(auth_mutex);
        
        ExecTrace::ProjectEntry project;
        project.project_id = next_project_id++;
        project.user_id = user_id;
        strncpy(project.name, project_name.c_str(), sizeof(project.name) - 1);
        
        // Generate API key
        std::string api_key = generate_api_key();
        strncpy(project.api_key, api_key.c_str(), sizeof(project.api_key) - 1);
        
        // Set default thresholds
        project.fast_threshold = 100;
        project.normal_threshold = 500;
        
        project_tree->insert(project);
        
        out_api_key = api_key;
        out_project_id = project.project_id;
        
        std::cout << "[AuthDB] Created project: " << project_name 
                  << " (ID: " << project.project_id << ", Key: " << api_key << ")" << std::endl;
        return true;
    }

    // Get project ID from API key (CRITICAL for multi-project support)
    bool get_project_id_from_api_key(const std::string& api_key, int& out_project_id) {
        std::lock_guard<std::mutex> lock(auth_mutex);
        
        // Get all projects from tree
        auto all_projects = project_tree->get_all_values();
        
        for (const auto& project : all_projects) {
            if (strcmp(project.api_key, api_key.c_str()) == 0 && !project.is_deleted) {
                out_project_id = project.project_id;
                std::cout << "[AuthDB] API key validated: Project " << out_project_id << std::endl;
                return true;
            }
        }
        
        std::cout << "[AuthDB] Invalid API key: " << api_key.substr(0, 12) << "..." << std::endl;
        return false;
    }
    
    // Get all projects for a user
    std::vector<ExecTrace::ProjectEntry> get_projects_by_user(int user_id) {
        std::lock_guard<std::mutex> lock(auth_mutex);
        
        std::vector<ExecTrace::ProjectEntry> user_projects;
        
        // Get all projects
        auto all_projects = project_tree->get_all_values();
        
        // Filter by user_id and exclude deleted projects
        for (const auto& project : all_projects) {
            if (project.user_id == user_id && !project.is_deleted) {
                user_projects.push_back(project);
            }
        }
        
        std::cout << "[AuthDB] Found " << user_projects.size() << " projects for user " << user_id << std::endl;
        return user_projects;
    }
    
    // Update project settings
    bool update_project_settings(int project_id, int fast_threshold, int normal_threshold) {
        std::lock_guard<std::mutex> lock(auth_mutex);
        
        // Find project
        ExecTrace::ProjectEntry search_key;
        search_key.project_id = project_id;
        auto results = project_tree->search(search_key);
        
        if (results.empty()) {
            std::cout << "[AuthDB] Project not found: " << project_id << std::endl;
            return false;
        }
        
        // Update thresholds
        ExecTrace::ProjectEntry project = results[0];
        project.fast_threshold = fast_threshold;
        project.normal_threshold = normal_threshold;
        
        // Re-insert (update in place)
        project_tree->insert(project);
        
        std::cout << "[AuthDB] Updated project " << project_id 
                  << " thresholds: " << fast_threshold << "/" << normal_threshold << "ms" << std::endl;
        return true;
    }
    
    // Delete project
    bool delete_project(int project_id) {
        std::lock_guard<std::mutex> lock(auth_mutex);
        
        // Find project
        ExecTrace::ProjectEntry search_key;
        search_key.project_id = project_id;
        auto results = project_tree->search(search_key);
        
        if (results.empty()) {
            std::cout << "[AuthDB] Project not found: " << project_id << std::endl;
            return false;
        }
        
        // Get the project
        ExecTrace::ProjectEntry project = results[0];
        
        // Mark as deleted
        project.is_deleted = true;
        
        // Re-insert to update (B-Tree insert will overwrite)
        project_tree->insert(project);
        
        std::cout << "[AuthDB] Project " << project_id << " marked as deleted" << std::endl;
        return true;
    }
    
    // ========== PERMISSION CHECKING SYSTEM ==========
    
    // Check if user has permission for an action
    bool has_permission(int user_id, const std::string& action, int resource_id = 0) {
        std::lock_guard<std::mutex> lock(auth_mutex);
        
        // Get user
        auto all_users = user_tree->get_all_values();
        ExecTrace::UserEntry* user = nullptr;
        
        for (auto& u : all_users) {
            if (u.user_id == user_id && u.is_active) {
                user = &u;
                break;
            }
        }
        
        if (!user) {
            std::cout << "[AuthDB] Permission denied: user not found or inactive" << std::endl;
            return false;
        }
        
        // Admins have all permissions
        if (user->role == ExecTrace::ROLE_ADMIN) {
            return true;
        }
        
        // Check specific permissions based on action
        if (action == "view_project" || action == "edit_project") {
            return is_project_owner(user_id, resource_id);
        }
        
        if (action == "delete_project") {
            return is_project_owner(user_id, resource_id);
        }
        
        if (action == "manage_users" || action == "assign_roles") {
            return false; // Only admins can do this
        }
        
        return false;
    }
    
    // Check if user owns a project
    bool is_project_owner(int user_id, int project_id) {
        auto all_projects = project_tree->get_all_values();
        for (const auto& project : all_projects) {
            if (project.project_id == project_id && 
                project.user_id == user_id && 
                !project.is_deleted) {
                return true;
            }
        }
        return false;
    }
    
    // Get user by ID
    bool get_user_by_id(int user_id, ExecTrace::UserEntry& out_user) {
        std::lock_guard<std::mutex> lock(auth_mutex);
        
        auto all_users = user_tree->get_all_values();
        for (const auto& user : all_users) {
            if (user.user_id == user_id && user.is_active) {
                out_user = user;
                return true;
            }
        }
        
        std::cout << "[AuthDB] User not found: " << user_id << std::endl;
        return false;
    }
    
    // ========== ADMIN USER MANAGEMENT ==========
    
    // Get all users (Admin only)
    std::vector<ExecTrace::UserEntry> get_all_users() {
        std::lock_guard<std::mutex> lock(auth_mutex);
        
        std::vector<ExecTrace::UserEntry> active_users;
        auto all_users = user_tree->get_all_values();
        
        for (const auto& user : all_users) {
            if (user.is_active) {
                active_users.push_back(user);
            }
        }
        
        std::cout << "[AuthDB] Found " << active_users.size() << " active users" << std::endl;
        return active_users;
    }
    
    // Update user role (Admin only)
    bool update_user_role(int user_id, int new_role) {
        std::lock_guard<std::mutex> lock(auth_mutex);
        
        // Prevent changing the first user (super admin)
        if (user_id == 1) {
            std::cout << "[AuthDB] Cannot change role of super admin (user_id=1)" << std::endl;
            return false;
        }
        
        // Get all users
        auto all_users = user_tree->get_all_values();
        bool found = false;
        ExecTrace::UserEntry updated_user;
        int old_role = 0;
        
        // Find the user and update their role
        for (auto& user : all_users) {
            if (user.user_id == user_id) {
                found = true;
                old_role = user.role;
                user.role = new_role;
                updated_user = user;
                break;
            }
        }
        
        if (!found) {
            std::cout << "[AuthDB] User not found: " << user_id << std::endl;
            return false;
        }
        
        // Rebuild the tree with updated user info
        std::vector<ExecTrace::UserEntry> new_user_list;
        for (const auto& user : all_users) {
            if (user.user_id == user_id) {
                new_user_list.push_back(updated_user);
            } else {
                new_user_list.push_back(user);
            }
        }
        
        rebuild_user_tree(new_user_list);
        
        const char* role_names[] = {"User", "Editor", "Admin"};
        std::cout << "[AuthDB] Updated user " << user_id 
                  << " role from " << role_names[old_role] 
                  << " to " << role_names[new_role] << std::endl;
        
        return true;
    }
    
    // Deactivate user (soft delete)
    bool deactivate_user(int user_id) {
        std::lock_guard<std::mutex> lock(auth_mutex);
        
        // Prevent deactivating super admin
        if (user_id == 1) {
            std::cout << "[AuthDB] Cannot deactivate super admin (user_id=1)" << std::endl;
            return false;
        }
        
        // Get all users
        auto all_users = user_tree->get_all_values();
        bool found = false;
        ExecTrace::UserEntry deactivated_user;
        
        // Find the user and deactivate
        for (auto& user : all_users) {
            if (user.user_id == user_id) {
                found = true;
                user.is_active = false;
                deactivated_user = user;
                break;
            }
        }
        
        if (!found) {
            return false;
        }
        
        // Rebuild the tree with deactivated user
        std::vector<ExecTrace::UserEntry> new_user_list;
        for (const auto& user : all_users) {
            if (user.user_id == user_id) {
                new_user_list.push_back(deactivated_user);
            } else {
                new_user_list.push_back(user);
            }
        }
        
        rebuild_user_tree(new_user_list);
        
        std::cout << "[AuthDB] Deactivated user: " << deactivated_user.username << std::endl;
        
        return true;
    }
};
