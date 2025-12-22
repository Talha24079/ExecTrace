#include <iostream>
#include <limits>
#include <csignal>
#include <vector>
#include <algorithm>
#include <set>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "../include/httplib.h"
#include "../include/btree/ExecTraceDB.hpp"
#include "../include/btree/AuthDB.hpp"

using namespace std;
namespace fs = std::filesystem;

ExecTraceDB* trace_db;
AuthDB* auth_db;
httplib::Server* global_svr = nullptr;

void signal_handler(int signal)
{
    if (global_svr) {
        cout << "\nStopping server..." << endl;
        global_svr->stop();
    }
}

string logs_to_json(const vector<TraceEntry>& logs)
{
    string json = "[";
    for (size_t i = 0; i < logs.size(); i++)
    {
        json += "{\"id\":" + to_string(logs[i].id) +
                ", \"project_id\":" + to_string(logs[i].project_id) +     
                ", \"version\":" + to_string(logs[i].func_call_count) +
                ", \"app_version\":\"" + string(logs[i].app_version) + "\"" +     
                ", \"func\":\"" + logs[i].process_name + "\"" +
                ", \"msg\":\"" + logs[i].message + "\"" +
                ", \"ram\":" + to_string(logs[i].ram_usage) +
                ", \"rom\":" + to_string(logs[i].rom_usage) +
                ", \"duration\":" + to_string(logs[i].duration) + "}";

        if (i < logs.size() - 1) json += ",";
    }
    json += "]";
    return json;
}

string read_file(const string& path) {
    ifstream file(path, ios::binary);
    if (file) {
        stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    return "";
}

int main(int argc, char* argv[])
{
    cout << "--- Initializing Secure Trace Server (SaaS Edition) ---" << endl;

    string db_folder = "data";
    if (!fs::exists(db_folder)) fs::create_directories(db_folder);

    trace_db = new ExecTraceDB(db_folder + "/sys_logs");
    auth_db = new AuthDB(db_folder + "/sys_users.db", db_folder + "/sys_projects.db");

    httplib::Server svr;
    global_svr = &svr;
    signal(SIGINT, signal_handler);

    svr.Get("/", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_content(read_file("frontend/index.html"), "text/html");
    });

    svr.Get("/workspace", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_content(read_file("frontend/workspace.html"), "text/html");
    });
    
    svr.Get("/dashboard", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_content(read_file("frontend/dashboard.html"), "text/html");
    });

    svr.Post("/register", [](const httplib::Request& req, httplib::Response& res) {
        if (req.has_param("username")) {
            string username = req.get_param_value("username");
            // Validate username length (max 31 chars for UserEntry)
            if (username.empty() || username.length() > 31) {
                res.status = 400;
                res.set_content("{\"error\": \"Username must be 1-31 characters\"}", "application/json");
                return;
            }
            string key = auth_db->register_user(username);
            res.set_content("{\"api_key\": \"" + key + "\"}", "application/json");
        } else {
            res.status = 400;
            res.set_content("{\"error\": \"Missing username parameter\"}", "application/json");
        }
    });

    svr.Post("/projects/create", [](const httplib::Request& req, httplib::Response& res) {
        string user_key = req.get_header_value("X-User-Key");
        UserEntry user;
        
        if (auth_db->validate_user_key(user_key, user)) {
            string name = req.get_param_value("name");
            // Validate project name length (max 31 chars for ProjectEntry)
            if (name.empty() || name.length() > 31) {
                res.status = 400;
                res.set_content("{\"error\": \"Project name must be 1-31 characters\"}", "application/json");
                return;
            }
            string proj_key = auth_db->create_project(user.user_id, name);
            res.set_content("{\"project_key\": \"" + proj_key + "\"}", "application/json");
        } else {
            res.status = 401;
            res.set_content("{\"error\": \"Invalid or missing X-User-Key\"}", "application/json");
        }
    });

    svr.Get("/projects/list", [](const httplib::Request& req, httplib::Response& res) {
        string user_key = req.get_header_value("X-User-Key");
        UserEntry user;
        
        if (auth_db->validate_user_key(user_key, user)) {
            vector<ProjectEntry> projs = auth_db->get_user_projects(user.user_id);
            string json = "[";
            for(size_t i=0; i<projs.size(); i++) {
                json += "{\"name\":\"" + string(projs[i].name) + "\", \"key\":\"" + string(projs[i].api_key) + "\"}";
                if(i < projs.size()-1) json += ",";
            }
            json += "]";
            res.set_content(json, "application/json");
        } else {
            res.status = 401;
        }
    });

    svr.Post("/projects/delete", [](const httplib::Request& req, httplib::Response& res) {
        string user_key = req.get_header_value("X-User-Key");
        string proj_key = req.get_param_value("project_key");
        
        UserEntry user;
        if (!auth_db->validate_user_key(user_key, user)) {
            res.status = 401;
            res.set_content("{\"error\": \"Invalid user key\"}", "application/json");
            return;
        }
        
        if (proj_key.empty()) {
            res.status = 400;
            res.set_content("{\"error\": \"Missing project_key parameter\"}", "application/json");
            return;
        }
        
        if (auth_db->delete_project(proj_key, user.user_id)) {
            res.set_content("{\"status\": \"deleted\"}", "application/json");
        } else {
            res.status = 403;
            res.set_content("{\"error\": \"Project not found or access denied\"}", "application/json");
        }
    });

    svr.Post("/projects/rename", [](const httplib::Request& req, httplib::Response& res) {
        string user_key = req.get_header_value("X-User-Key");
        string proj_key = req.get_param_value("project_key");
        string new_name = req.get_param_value("new_name");
        
        UserEntry user;
        if (!auth_db->validate_user_key(user_key, user)) {
            res.status = 401;
            res.set_content("{\"error\": \"Invalid user key\"}", "application/json");
            return;
        }
        
        if (proj_key.empty() || new_name.empty()) {
            res.status = 400;
            res.set_content("{\"error\": \"Missing required parameters\"}", "application/json");
            return;
        }
        
        if (auth_db->rename_project(proj_key, user.user_id, new_name)) {
            res.set_content("{\"status\": \"renamed\"}", "application/json");
        } else {
            res.status = 400;
            res.set_content("{\"error\": \"Failed to rename (invalid name or access denied)\"}", "application/json");
        }
    });

    svr.Post("/projects/regenerate", [](const httplib::Request& req, httplib::Response& res) {
        string user_key = req.get_header_value("X-User-Key");
        string old_proj_key = req.get_param_value("project_key");
        
        UserEntry user;
        if (!auth_db->validate_user_key(user_key, user)) {
            res.status = 401;
            res.set_content("{\"error\": \"Invalid user key\"}", "application/json");
            return;
        }
        
        if (old_proj_key.empty()) {
            res.status = 400;
            res.set_content("{\"error\": \"Missing project_key parameter\"}", "application/json");
            return;
        }
        
        string new_key = auth_db->regenerate_project_key(old_proj_key, user.user_id);
        if (!new_key.empty()) {
            res.set_content("{\"new_key\": \"" + new_key + "\"}", "application/json");
        } else {
            res.status = 403;
            res.set_content("{\"error\": \"Failed to regenerate key (project not found or access denied)\"}", "application/json");
        }
    });

    svr.Post("/log", [&](const httplib::Request& req, httplib::Response& res) {
        string key = req.get_header_value("X-API-Key");
        ProjectEntry proj;

        if (!auth_db->validate_project_key(key, proj)) {
            res.status = 403;
            res.set_content("{\"error\": \"Invalid or missing X-API-Key\"}", "application/json");
            return;
        }

        if (req.has_param("func") && req.has_param("msg") && 
            req.has_param("duration") && req.has_param("ram")) {
            
            string func = req.get_param_value("func");
            string msg = req.get_param_value("msg");
            string app_version = req.has_param("app_version") ? req.get_param_value("app_version") : "v1.0.0";
            
            // Validate field lengths (TraceEntry has char[32] for func, char[64] for msg, char[16] for app_version)
            if (func.empty() || func.length() > 31 || msg.length() > 63 || app_version.length() > 15) {
                res.status = 400;
                res.set_content("{\"error\": \"Invalid field lengths\"}", "application/json");
                return;
            }
            
            trace_db->log_event(
                proj.project_id, 
                func.c_str(), 
                msg.c_str(),
                app_version.c_str(),
                stoull(req.get_param_value("duration")), 
                stoull(req.get_param_value("ram")), 
                0
            );
            res.set_content("{\"status\":\"ok\"}", "application/json");
        } else {
            res.status = 400;
            res.set_content("{\"error\": \"Missing required parameters\"}", "application/json");
        }
    });
    
    svr.Get("/query", [&](const httplib::Request& req, httplib::Response& res) {
        string key = req.get_header_value("X-API-Key");
        ProjectEntry proj;

        if (auth_db->validate_project_key(key, proj)) {
            vector<TraceEntry> logs = trace_db->query_range(proj.project_id, 0, INT_MAX);
            res.set_content(logs_to_json(logs), "application/json");
        } else {
            res.status = 403;
        }
    });

    // Smart Query Endpoint: Server-side sorting, filtering, and pagination
    svr.Get("/query/advanced", [&](const httplib::Request& req, httplib::Response& res) {
        string key = req.get_header_value("X-API-Key");
        ProjectEntry proj;

        if (!auth_db->validate_project_key(key, proj)) {
            res.status = 403;
            res.set_content("{\"error\": \"Invalid or missing X-API-Key\"}", "application/json");
            return;
        }

        // Parse query parameters with defaults
        string sort_by = req.has_param("sort_by") ? req.get_param_value("sort_by") : "id";
        string order = req.has_param("order") ? req.get_param_value("order") : "desc";
        int limit = req.has_param("limit") ? stoi(req.get_param_value("limit")) : 50;
        int offset = req.has_param("offset") ? stoi(req.get_param_value("offset")) : 0;

        // Call smart_query from ExecTraceDB
        vector<TraceEntry> logs = trace_db->smart_query(proj.project_id, sort_by, order, limit, offset);
        res.set_content(logs_to_json(logs), "application/json");
    });

    // Statistics Endpoint: Aggregated metrics by function or version
    svr.Get("/stats", [&](const httplib::Request& req, httplib::Response& res) {
        string key = req.get_header_value("X-API-Key");
        ProjectEntry proj;

        if (!auth_db->validate_project_key(key, proj)) {
            res.status = 403;
            res.set_content("{\"error\": \"Invalid or missing X-API-Key\"}", "application/json");
            return;
        }

        // Parse group_by parameter (default: "func")
        string group_by = req.has_param("group_by") ? req.get_param_value("group_by") : "func";

        // Compute statistics
        auto stats = trace_db->compute_stats(proj.project_id, group_by);

        // Convert map to JSON
        string json = "{";
        bool first = true;
        for (const auto& [key, stat] : stats) {
            if (!first) json += ",";
            first = false;
            
            json += "\"" + key + "\": {";
            json += "\"avg_duration\":" + to_string(stat.avg_duration) + ",";
            json += "\"max_ram\":" + to_string(stat.max_ram) + ",";
            json += "\"min_duration\":" + to_string(stat.min_duration) + ",";
            json += "\"count\":" + to_string(stat.count);
            json += "}";
        }
        json += "}";

        res.set_content(json, "application/json");
    });

    cout << "Server listening on http://localhost:8080" << endl;
    svr.listen("0.0.0.0", 8080);
    return 0;
}