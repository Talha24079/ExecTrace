#pragma once
#include <cstring>
#include <ctime>
#include <cstdint>

namespace ExecTrace {

// TraceEntry - represents a single trace log
struct TraceEntry {
    int id;
    int project_id;
    char func[128];
    char message[256];
    char app_version[32];
    uint64_t duration;
    uint64_t ram_usage;
    time_t timestamp;

    TraceEntry() : id(0), project_id(0), duration(0), ram_usage(0), timestamp(0) {
        memset(func, 0, sizeof(func));
        memset(message, 0, sizeof(message));
        memset(app_version, 0, sizeof(app_version));
    }

    TraceEntry(int entry_id, int proj_id, const char* function, 
               const char* msg, const char* version, 
               uint64_t dur, uint64_t ram) 
        : id(entry_id), project_id(proj_id), duration(dur), 
          ram_usage(ram), timestamp(time(nullptr)) {
        
        // Initialize arrays
        memset(func, 0, sizeof(func));
        memset(message, 0, sizeof(message));
        memset(app_version, 0, sizeof(app_version));
        
        // Safe string copies with null checks
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

    bool operator>(const TraceEntry& other) const {
        return id > other.id;
    }
};

// UserEntry - represents a user account
struct UserEntry {
    int user_id;
    char email[128];
    uint64_t email_hash;
    char password_hash[65];
    char username[64];

    UserEntry() : user_id(0), email_hash(0) {
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

// ProjectEntry - represents a project with API key
struct ProjectEntry {
    int project_id;
    int user_id;
    char name[128];
    char api_key[64];
    int fast_threshold;
    int normal_threshold;

    ProjectEntry() : project_id(0), user_id(0), fast_threshold(100), normal_threshold(500) {
        memset(name, 0, sizeof(name));
        memset(api_key, 0, sizeof(api_key));
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

} // namespace ExecTrace
