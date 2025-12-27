#pragma once
#include <cstring>
#include <ctime>
#include <cstdint>

namespace ExecTrace {

struct TraceEntry {
    int id;
    int project_id;
    char func[128];
    char message[256];
    char app_version[32];
    uint64_t duration;
    uint64_t ram_usage;
    time_t timestamp;
    bool is_deleted; 

    TraceEntry() : id(0), project_id(0), duration(0), ram_usage(0), timestamp(0), is_deleted(false) {
        memset(func, 0, sizeof(func));
        memset(message, 0, sizeof(message));
        memset(app_version, 0, sizeof(app_version));
    }

    TraceEntry(int entry_id, int proj_id, const char* function, 
               const char* msg, const char* version, 
               uint64_t dur, uint64_t ram) 
        : id(entry_id), project_id(proj_id), duration(dur), 
          ram_usage(ram), timestamp(time(nullptr)), is_deleted(false) {

        memset(func, 0, sizeof(func));
        memset(message, 0, sizeof(message));
        memset(app_version, 0, sizeof(app_version));

        if (function) {
            strncpy(func, function, sizeof(func) - 1);
            func[sizeof(func) - 1] = '\0';
        }
        
        if (msg) {
            strncpy(message, msg, sizeof(message) - 1);
            message[sizeof(message) - 1] = '\0';
        }
        
        if (version) {
            strncpy(app_version, version, sizeof(app_version) - 1);
            app_version[sizeof(app_version) - 1] = '\0';
        }
    }

    bool operator<(const TraceEntry& other) const {
        return id < other.id;
    }

    bool operator==(const TraceEntry& other) const {
        return id == other.id;
    }

    bool is_valid() const {
        return id > 0 && 
               project_id > 0 &&
               duration < 3600000 &&  
               ram_usage < 104857600 &&  
               func[0] != '\0' &&  
               !is_deleted;
    }

    bool operator>(const TraceEntry& other) const {
        return id > other.id;
    }
};

enum UserRole {
    ROLE_USER = 0,    
    ROLE_EDITOR = 1,  
    ROLE_ADMIN = 2    
};

struct UserEntry {
    int user_id;
    char email[128];
    uint64_t email_hash;
    char password_hash[65];
    char username[64];
    int role;         
    bool is_active;   
    time_t created_at; 

    UserEntry() : user_id(0), email_hash(0), role(ROLE_USER), 
                  is_active(true), created_at(time(nullptr)) {
        memset(email, 0, sizeof(email));
        memset(password_hash, 0, sizeof(password_hash));
        memset(username, 0, sizeof(username));
    }

    bool operator<(const UserEntry& other) const {
        return email_hash < other.email_hash;
    }

    bool operator==(const UserEntry& other) const {
        return email_hash == other.email_hash;
    }

    bool operator>(const UserEntry& other) const {
        return email_hash > other.email_hash;
    }
};

struct ProjectEntry  { 
    int project_id;
    int user_id;
    char name[128];
    char api_key[64];
    int fast_threshold;
    int normal_threshold;
    bool is_deleted;  
    
    ProjectEntry() : project_id(0), user_id(0), fast_threshold(100), 
                    normal_threshold(500), is_deleted(false) {
        memset(name, 0, sizeof(name));
        memset(api_key, 0, sizeof(api_key));
    }
    
    ProjectEntry(int pid, int uid, const std::string& n, const std::string& key) 
        : project_id(pid), user_id(uid), fast_threshold(100), 
          normal_threshold(500), is_deleted(false) {
        strncpy(name, n.c_str(), sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0'; 
        strncpy(api_key, key.c_str(), sizeof(api_key) - 1);
        api_key[sizeof(api_key) - 1] = '\0'; 
    }

    bool operator<(const ProjectEntry& other) const {
        return project_id < other.project_id;
    }
    
    bool operator==(const ProjectEntry& other) const {
        return project_id == other.project_id;
    }

    bool operator>(const ProjectEntry& other) const {
        return project_id > other.project_id;
    }
};

struct ProjectCollaborator {
    int id;
    int project_id;
    int user_id;
    int role;  
    time_t added_at;
    
    ProjectCollaborator() : id(0), project_id(0), user_id(0), 
                           role(1), added_at(time(nullptr)) {}
    
    ProjectCollaborator(int pid, int uid, int r) 
        : id(0), project_id(pid), user_id(uid), role(r), 
          added_at(time(nullptr)) {}
    
    bool operator<(const ProjectCollaborator& other) const {
        return id < other.id;
    }
    
    bool operator==(const ProjectCollaborator& other) const {
        return id == other.id;
    }
    
    bool operator>(const ProjectCollaborator& other) const {
        return id > other.id;
    }
};

struct AuditLog {
    int id;
    int user_id;        
    char action[256];   
    char details[512];  
    time_t timestamp;
    
    AuditLog() : id(0), user_id(0), timestamp(time(nullptr)) {
        memset(action, 0, sizeof(action));
        memset(details, 0, sizeof(details));
    }
    
    AuditLog(int uid, const char* act, const char* det) 
        : id(0), user_id(uid), timestamp(time(nullptr)) {
        memset(action, 0, sizeof(action));
        memset(details, 0, sizeof(details));
        
        if (act) {
            strncpy(action, act, sizeof(action) - 1);
            action[sizeof(action) - 1] = '\0';
        }
        
        if (det) {
            strncpy(details, det, sizeof(details) - 1);
            details[sizeof(details) - 1] = '\0';
        }
    }
    
    bool operator<(const AuditLog& other) const {
        return id < other.id;
    }
    
    bool operator==(const AuditLog& other) const {
        return id == other.id;
    }
    
    bool operator>(const AuditLog& other) const {
        return id > other.id;
    }
};

} 
