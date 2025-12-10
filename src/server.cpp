#include <iostream>
#include <limits>
#include <csignal>
#include <vector>
#include <algorithm>
#include <set>
#include "../include/httplib.h"
#include "../include/btree/ExecTraceDB.hpp"
#include "../include/btree/AuthDB.hpp"

using namespace std;

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

vector<int> intersect_ids(vector<int> v1, vector<int> v2)
{
    vector<int> intersection;
    sort(v1.begin(), v1.end());
    sort(v2.begin(), v2.end());
    set_intersection(v1.begin(), v1.end(), v2.begin(), v2.end(), back_inserter(intersection));
    return intersection;
}

string logs_to_json(const vector<TraceEntry>& logs)
{
    string json = "[";
    for (size_t i = 0; i < logs.size(); i++)
    {
        json += "{\"id\":" + to_string(logs[i].id) +
                ", \"user_id\":" + to_string(logs[i].user_id) +     
                ", \"version\":" + to_string(logs[i].version) +     
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

int main()
{
    cout << "--- Initializing Secure Trace Server ---" << endl;

    trace_db = new ExecTraceDB("sys_logs");
    auth_db = new AuthDB("sys_users.db");

    httplib::Server svr;
    global_svr = &svr;

    signal(SIGINT, signal_handler);

    auto get_user_id = [](const httplib::Request& req, httplib::Response& res) -> int
    {
        if (!req.has_header("X-API-Key")) {
            res.status = 401;
            res.set_content("{\"error\": \"Missing X-API-Key header\"}", "application/json");
            return -1;
        }

        string key = req.get_header_value("X-API-Key");
        cout << "[DEBUG] Auth Check. Received Key: '" << key << "'" << endl;
        UserEntry user;

        if (!auth_db->validate_key(key, user)) {
            res.status = 403;
            res.set_content("{\"error\": \"Invalid API Key\"}", "application/json");
            return -1;
        }

        return user.user_id; 
    };

    auto check_auth_only = [&](const httplib::Request& req, httplib::Response& res) -> bool {
        return get_user_id(req, res) != -1;
    };

    svr.Get("/status", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content("{\"status\": \"running\", \"auth\": \"enabled\"}", "application/json");
    });

    svr.Post("/register", [](const httplib::Request& req, httplib::Response& res) {
        if (req.has_param("username")) {
            string username = req.get_param_value("username");
            string key = auth_db->register_user(username);
            cout << "Registered user: " << username << endl;
            res.set_content("{\"username\": \"" + username + "\", \"api_key\": \"" + key + "\"}", "application/json");
        } else {
            res.status = 400;
            res.set_content("{\"error\": \"Missing username param\"}", "application/json");
        }
    });

    svr.Post("/log", [&](const httplib::Request& req, httplib::Response& res) {
        int user_id = get_user_id(req, res); 
        if (user_id == -1) return;

        if (req.has_param("func")) {
            string func = req.get_param_value("func");
            string msg = req.get_param_value("msg");
            uint64_t dur = stoull(req.get_param_value("duration"));
            uint64_t ram = stoull(req.get_param_value("ram"));
            uint64_t rom = stoull(req.get_param_value("rom"));

            int new_id = trace_db->log_event(user_id, func.c_str(), msg.c_str(), dur, ram, rom);

            cout << "Log saved. ID: " << new_id << " (User: " << user_id << ")" << endl;
            res.set_content("{\"result\": \"saved\", \"id\": " + to_string(new_id) + "}", "application/json");
        } else {
            res.status = 400;
            res.set_content("{\"result\": \"error\", \"msg\": \"missing func param\"}", "application/json");
        }
    });

    svr.Get("/query", [&](const httplib::Request& req, httplib::Response& res) {
        if (!check_auth_only(req, res)) return;
        int start = req.has_param("start") ? stoi(req.get_param_value("start")) : 0;
        int end = req.has_param("end") ? stoi(req.get_param_value("end")) : numeric_limits<int>::max();
        vector<TraceEntry> logs = trace_db->query_range(start, end);
        res.set_content(logs_to_json(logs), "application/json");
    });

    svr.Get("/query/ram", [&](const httplib::Request& req, httplib::Response& res) {
        if (!check_auth_only(req, res)) return;
        int min_ram = req.has_param("min") ? stoi(req.get_param_value("min")) : 0;
        int max_ram = req.has_param("max") ? stoi(req.get_param_value("max")) : numeric_limits<int>::max();
        vector<TraceEntry> logs = trace_db->query_by_ram(min_ram, max_ram);
        res.set_content(logs_to_json(logs), "application/json");
    });

    svr.Get("/query/duration", [&](const httplib::Request& req, httplib::Response& res) {
        if (!check_auth_only(req, res)) return;
        int min_dur = req.has_param("min") ? stoi(req.get_param_value("min")) : 0;
        int max_dur = req.has_param("max") ? stoi(req.get_param_value("max")) : numeric_limits<int>::max();
        vector<TraceEntry> logs = trace_db->query_by_duration(min_dur, max_dur);
        res.set_content(logs_to_json(logs), "application/json");
    });

    svr.Get("/query/heavy", [&](const httplib::Request& req, httplib::Response& res) {
        if (!check_auth_only(req, res)) return;

        int min_ram = req.has_param("min_ram") ? stoi(req.get_param_value("min_ram")) : 0;
        int min_dur = req.has_param("min_dur") ? stoi(req.get_param_value("min_dur")) : 0;

        vector<TraceEntry> ram_logs = trace_db->query_by_ram(min_ram, numeric_limits<int>::max());
        vector<TraceEntry> dur_logs = trace_db->query_by_duration(min_dur, numeric_limits<int>::max());

        vector<int> ram_ids; for (const auto& log : ram_logs) ram_ids.push_back(log.id);
        vector<int> dur_ids; for (const auto& log : dur_logs) dur_ids.push_back(log.id);

        vector<int> heavy_ids = intersect_ids(ram_ids, dur_ids);

        vector<TraceEntry> final_results;
        set<int> seen;
        for (const auto& log : ram_logs) {
            for (int id : heavy_ids) {
                if (log.id == id && seen.find(id) == seen.end()) {
                    final_results.push_back(log);
                    seen.insert(id);
                    break;
                }
            }
        }
        res.set_content(logs_to_json(final_results), "application/json");
    });

    cout << "Server listening on http://localhost:8080" << endl;
    svr.listen("0.0.0.0", 8080);
    delete trace_db;
    delete auth_db;
    return 0;
}