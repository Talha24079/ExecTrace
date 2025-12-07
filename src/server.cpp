#include <iostream>
#include "httplib.h"
#include "btree/ExecTraceDB.hpp"

using namespace std;

ExecTraceDB* db;

int main()
{
    cout << "--- Initializing ExecTrace Server ---" << endl;
    
    db = new ExecTraceDB("prod_trace.db");
    
    httplib::Server svr;

    svr.Get("/status", [](const httplib::Request& req, httplib::Response& res)
    {
        res.set_content("{\"status\": \"running\", \"db\": \"connected\"}", "application/json");
    });

    svr.Post("/log", [](const httplib::Request& req, httplib::Response& res)
    {
        cout << "Received Log Request" << endl;
        
        if (req.has_param("id"))
        {
            int id = stoi(req.get_param_value("id"));
            string func = req.get_param_value("func");
            string msg = req.get_param_value("msg");
            uint64_t dur = stoull(req.get_param_value("duration"));
            uint64_t ram = stoull(req.get_param_value("ram"));
            uint64_t rom = stoull(req.get_param_value("rom"));

            db->log_event(id, func.c_str(), msg.c_str(), dur, ram, rom);
            res.set_content("{\"result\": \"saved\"}", "application/json");
        }
        else
        {
            res.set_content("{\"result\": \"error\", \"msg\": \"missing params\"}", "application/json");
        }
    });

    svr.Get("/query", [](const httplib::Request& req, httplib::Response& res)
    {
        if (req.has_param("start") && req.has_param("end"))
        {
            int start = stoi(req.get_param_value("start"));
            int end = stoi(req.get_param_value("end"));
            
            vector<TraceEntry> logs = db->query_range(start, end);
            
            string json = "[";
            for(size_t i = 0; i < logs.size(); i++)
            {
                json += "{\"id\":" + to_string(logs[i].id) + 
                        ", \"func\":\"" + logs[i].process_name + "\"" +
                        ", \"ram\":" + to_string(logs[i].ram_usage) + "}";
                if (i < logs.size() - 1) json += ",";
            }
            json += "]";
            
            res.set_content(json, "application/json");
        }
    });

    cout << "Server listening on http://localhost:8080" << endl;
    svr.listen("0.0.0.0", 8080);

    delete db;
    return 0;
}