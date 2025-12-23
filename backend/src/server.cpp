#include "../include/httplib.h"
#include "../include/json.hpp"
#include "../include/Database.hpp"
#include "../include/Models.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;
using json = nlohmann::json;

AuthDB* auth_db;
ExecTraceDB* trace_db;

string read_file(const string& path) {
    ifstream file(path);
    if (!file.is_open()) return "";
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

string logs_to_json(const vector<ExecTrace::TraceEntry>& logs) {
    json j = json::array();
    for (const auto& log : logs) {
        j.push_back({
            {"id", log.id},
            {"func", log.func},
            {"message", log.message},
            {"app_version", log.app_version},
            {"duration", log.duration},
            {"ram", log.ram_usage},
            {"timestamp", log.timestamp}
        });
    }
    return j.dump();
}

int main() {
    cout << "--- ExecTrace Server ---" << endl;

    cout << "Initializing AuthDB..." << endl;
    auth_db = new AuthDB("data/users.db", "data/projects.db");
    cout << "AuthDB ready" << endl;
    
    cout << "Initializing ExecTraceDB..." << endl;
    trace_db = new ExecTraceDB("data/traces.db");
    cout << "ExecTraceDB ready" << endl;

    httplib::Server svr;

    svr.Get("/", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_content(read_file("../frontend/index.html"), "text/html");
    });

    svr.Get("/register", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_content(read_file("../frontend/register.html"), "text/html");
    });

    svr.Get("/login", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_content(read_file("../frontend/login.html"), "text/html");
    });

    svr.Get("/workspace", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_content(read_file("../frontend/workspace.html"), "text/html");
    });

    svr.Get("/dashboard", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_content(read_file("../frontend/dashboard.html"), "text/html");
    });

    svr.Get("/docs", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_content(read_file("../frontend/docs.html"), "text/html");
    });

    svr.Post("/api/auth/register", [](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("email") || !req.has_param("password") || !req.has_param("username")) {
            res.status = 400;
            res.set_content("{\"error\": \"Missing required fields\"}", "application/json");
            return;
        }

        string email = req.get_param_value("email");
        string password = req.get_param_value("password");
        string username = req.get_param_value("username");
        string error;

        if (auth_db->register_user(email, password, username, error)) {
            res.set_content("{\"success\": true}", "application/json");
        } else {
            res.status = 400;
            res.set_content("{\"error\": \"" + error + "\"}", "application/json");
        }
    });

    svr.Post("/api/auth/login", [](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("email") || !req.has_param("password")) {
            res.status = 400;
            res.set_content("{\"error\": \"Missing credentials\"}", "application/json");
            return;
        }

        string email = req.get_param_value("email");
        string password = req.get_param_value("password");
        string error;
        int user_id;
        string username;

        if (auth_db->login_user(email, password, user_id, username, error)) {
            json response = {
                {"success", true},
                {"user_id", user_id},
                {"username", username}
            };
            res.set_content(response.dump(), "application/json");
        } else {
            res.status = 401;
            res.set_content("{\"error\": \"" + error + "\"}", "application/json");
        }
    });

    svr.Post("/api/projects/create", [](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("user_id") || !req.has_param("name")) {
            res.status = 400;
            res.set_content("{\"error\": \"Missing parameters\"}", "application/json");
            return;
        }

        int user_id = stoi(req.get_param_value("user_id"));
        string name = req.get_param_value("name");
        
        string proj_key = auth_db->create_project(user_id, name);
        
        json response = {
            {"project_key", proj_key},
            {"name", name}
        };
        res.set_content(response.dump(), "application/json");
    });

    svr.Get("/api/projects/list", [](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("user_id")) {
            res.status = 400;
            res.set_content("{\"error\": \"Missing user_id\"}", "application/json");
            return;
        }

        int user_id = stoi(req.get_param_value("user_id"));
        vector<ExecTrace::ProjectEntry> projs = auth_db->get_user_projects(user_id);
        
        json j = json::array();
        for(const auto& p : projs) {
            j.push_back({
                {"id", p.project_id},
                {"name", p.name},
                {"api_key", p.api_key},
                {"created_at", p.created_at}
            });
        }
        res.set_content(j.dump(), "application/json");
    });

    svr.Post("/log", [](const httplib::Request& req, httplib::Response& res) {
        string key = req.get_header_value("X-API-Key");
        ExecTrace::ProjectEntry proj;

        if (!auth_db->validate_project_key(key, proj)) {
            res.status = 403;
            res.set_content("{\"error\": \"Invalid API Key\"}", "application/json");
            return;
        }

        if (!req.has_param("func") || !req.has_param("duration") || !req.has_param("ram")) {
            res.status = 400;
            res.set_content("{\"error\": \"Missing parameters\"}", "application/json");
            return;
        }

        string func = req.get_param_value("func");
        string msg = req.has_param("msg") ? req.get_param_value("msg") : "";
        string app_version = req.has_param("app_version") ? req.get_param_value("app_version") : "v1.0.0";
        uint64_t duration = stoull(req.get_param_value("duration"));
        uint64_t ram = stoull(req.get_param_value("ram"));

        trace_db->log_event(proj.project_id, func.c_str(), msg.c_str(), app_version.c_str(), duration, ram);
        res.set_content("{\"status\": \"ok\"}", "application/json");
    });

    svr.Get("/query/advanced", [](const httplib::Request& req, httplib::Response& res) {
        string key = req.get_header_value("X-API-Key");
        ExecTrace::ProjectEntry proj;

        if (!auth_db->validate_project_key(key, proj)) {
            res.status = 403;
            res.set_content("{\"error\": \"Invalid API Key\"}", "application/json");
            return;
        }

        int limit = req.has_param("limit") ? stoi(req.get_param_value("limit")) : 50;
        
        vector<ExecTrace::TraceEntry> logs = trace_db->query_range(proj.project_id, 0, INT_MAX);
        
        if (logs.size() > limit) {
            vector<ExecTrace::TraceEntry> limited(logs.end() - limit, logs.end());
            res.set_content(logs_to_json(limited), "application/json");
        } else {
            res.set_content(logs_to_json(logs), "application/json");
        }
    });

    svr.Get("/api/health", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content("{\"status\": \"ok\"}", "application/json");
    });

    cout << "Server listening on http://localhost:8080" << endl;
    svr.listen("0.0.0.0", 8080);

    delete auth_db;
    delete trace_db;

    return 0;
}
