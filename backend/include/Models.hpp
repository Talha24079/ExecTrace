#pragma once
#include <cstring>
#include <ctime>
#include <functional>

namespace ExecTrace {

inline size_t SimpleHash(const std::string& str) {
    return std::hash<std::string>{}(str);
}

struct UserEntry {
    size_t email_hash;
    char email[128];
    char password[64];
    int user_id;
    char username[32];
    long created_at;

    UserEntry() : email_hash(0), user_id(0), created_at(0) {
        memset(email, 0, sizeof(email));
        memset(password, 0, sizeof(password));
        memset(username, 0, sizeof(username));
    }

    UserEntry(const char* mail, const char* pass, const char* name, int id) {
        email_hash = SimpleHash(mail);
        user_id = id;
        created_at = time(nullptr);
        
        strncpy(email, mail, sizeof(email) - 1);
        email[sizeof(email) - 1] = '\0';
        
        strncpy(password, pass, sizeof(password) - 1);
        password[sizeof(password) - 1] = '\0';
        
        strncpy(username, name, sizeof(username) - 1);
        username[sizeof(username) - 1] = '\0';
    }

    bool operator<(const UserEntry& other) const {
        return email_hash < other.email_hash;
    }

    bool operator>(const UserEntry& other) const {
        return email_hash > other.email_hash;
    }

    bool operator==(const UserEntry& other) const {
        return email_hash == other.email_hash;
    }

    bool operator>=(const UserEntry& other) const {
        return email_hash >= other.email_hash;
    }

    bool operator<=(const UserEntry& other) const {
        return email_hash <= other.email_hash;
    }
};

struct ProjectEntry {
    char api_key[64];
    int project_id;
    int owner_id;
    char name[32];
    long created_at;

    ProjectEntry() : project_id(0), owner_id(0), created_at(0) {
        memset(api_key, 0, sizeof(api_key));
        memset(name, 0, sizeof(name));
    }

    ProjectEntry(const char* key, int proj_id, int owner, const char* proj_name) {
        project_id = proj_id;
        owner_id = owner;
        created_at = time(nullptr);
        
        strncpy(api_key, key, sizeof(api_key) - 1);
        api_key[sizeof(api_key) - 1] = '\0';
        
        strncpy(name, proj_name, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
    }

    bool operator<(const ProjectEntry& other) const {
        return strcmp(api_key, other.api_key) < 0;
    }

    bool operator>(const ProjectEntry& other) const {
        return strcmp(api_key, other.api_key) > 0;
    }

    bool operator==(const ProjectEntry& other) const {
        return strcmp(api_key, other.api_key) == 0;
    }

    bool operator>=(const ProjectEntry& other) const {
        return strcmp(api_key, other.api_key) >= 0;
    }

    bool operator<=(const ProjectEntry& other) const {
        return strcmp(api_key, other.api_key) <= 0;
    }
};

struct TraceEntry {
    int id;
    int project_id;
    char func[64];
    char message[128];
    char app_version[16];
    uint64_t duration;
    uint64_t ram_usage;
    long timestamp;

    TraceEntry() : id(0), project_id(0), duration(0), ram_usage(0), timestamp(0) {
        memset(func, 0, sizeof(func));
        memset(message, 0, sizeof(message));
        memset(app_version, 0, sizeof(app_version));
    }

    TraceEntry(int entry_id, int proj_id, const char* function, const char* msg,
               const char* version, uint64_t dur, uint64_t ram) {
        id = entry_id;
        project_id = proj_id;
        duration = dur;
        ram_usage = ram;
        timestamp = time(nullptr);
        
        strncpy(func, function, sizeof(func) - 1);
        func[sizeof(func) - 1] = '\0';
        
        strncpy(message, msg, sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';
        
        strncpy(app_version, version, sizeof(app_version) - 1);
        app_version[sizeof(app_version) - 1] = '\0';
    }

    bool operator<(const TraceEntry& other) const {
        return id < other.id;
    }

    bool operator>(const TraceEntry& other) const {
        return id > other.id;
    }

    bool operator==(const TraceEntry& other) const {
        return id == other.id;
    }

    bool operator>=(const TraceEntry& other) const {
        return id >= other.id;
    }

    bool operator<=(const TraceEntry& other) const {
        return id <= other.id;
    }
};

}
