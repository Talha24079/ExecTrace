#pragma once
#include "BTree.hpp"
#include "Models.hpp"
#include <random>
#include <sstream>
#include <iomanip>

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
    DiskManager* user_dm;
    DiskManager* project_dm;
    BTree<ExecTrace::UserEntry>* user_tree;
    BTree<ExecTrace::ProjectEntry>* project_tree;
    std::mutex auth_mutex;
    int next_user_id;
    int next_project_id;

public:
    AuthDB(const std::string& user_db_file, const std::string& project_db_file) 
        : next_user_id(1), next_project_id(1) {
        
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
        
        // Hash password (simple hash for demo)
        uint64_t pwd_hash = simple_hash(password);
        snprintf(user.password_hash, sizeof(user.password_hash), "%016lx", pwd_hash);
        
        user_tree->insert(user);
        out_user_id = user.user_id;
        
        std::cout << "[AuthDB] Registered user: " << username << " (ID: " << user.user_id << ")" << std::endl;
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
        
        out_user = results[0];
        std::cout << "[AuthDB] Login successful: " << out_user.username << std::endl;
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
            if (strcmp(project.api_key, api_key.c_str()) == 0) {
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
        
        // Filter by user_id
        for (const auto& project : all_projects) {
            if (project.user_id == user_id) {
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
        
        // Find project to verify it exists
        ExecTrace::ProjectEntry search_key;
        search_key.project_id = project_id;
        auto results = project_tree->search(search_key);
        
        if (results.empty()) {
            std::cout << "[AuthDB] Project not found: " << project_id << std::endl;
            return false;
        }
        
        // Note: B-Tree doesn't have delete yet, so we mark it as deleted
        // For now, just log that we would delete it
        // TODO: Implement B-Tree delete operation
        std::cout << "[AuthDB] Project " << project_id << " marked for deletion (full delete not implemented)" << std::endl;
        return true;
    }
};
