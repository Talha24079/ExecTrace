#include <iostream>
#include <string>
#include <vector>
#include "../include/httplib.h"
#include "../include/json.hpp"

using namespace std;
using json = nlohmann::json;

const char* SERVER_IP = "127.0.0.1";
const int SERVER_PORT = 8080;

void print_help()
{
    cout << "Usage:" << endl;
    cout << "  ./et-cli register <username>" << endl;
    cout << "  ./et-cli log <api_key> <func_name> <ram_mb> <duration_ms>" << endl;
    cout << "  ./et-cli query <api_key> ram <min> <max>" << endl;
    cout << "  ./et-cli query <api_key> duration <min> <max>" << endl;
}

void register_user(const string& username)
{
    httplib::Client cli(SERVER_IP, SERVER_PORT);
    
    string path = "/register?username=" + username;

    auto res = cli.Post(path.c_str());

    if (res && res->status == 200)
    {
        try {
            auto json_res = json::parse(res->body);
            cout << "Success! API Key: " << json_res["api_key"] << endl;
        } catch (...) {
            cout << "Error: Invalid JSON response from server." << endl;
        }
    }
    else
    {
        cout << "Error registering user. Status: " << (res ? to_string(res->status) : "Connection Failed") << endl;
    }
}

void log_trace(const string& api_key, const string& func_name, int ram, int dur)
{
    httplib::Client cli(SERVER_IP, SERVER_PORT);

    httplib::Params params;
    params.emplace("func", func_name);
    params.emplace("msg", "CLI Log Entry"); 
    params.emplace("ram", to_string(ram));
    params.emplace("duration", to_string(dur));
    params.emplace("rom", "0"); 

    httplib::Headers headers = { {"X-API-KEY", api_key} };

    auto res = cli.Post("/log", headers, params);

    if (res && res->status == 200)
    {
        cout << "Log saved successfully." << endl;
    }
    else
    {
        cout << "Error saving log. Status: " << (res ? to_string(res->status) : "Connection Failed") << endl;
        if(res) cout << "Server said: " << res->body << endl;
    }
}

void query_logs(const string& api_key, const string& type, int min_val, int max_val)
{
    httplib::Client cli(SERVER_IP, SERVER_PORT);
    
    string path = "/query/" + type + "?min=" + to_string(min_val) + "&max=" + to_string(max_val);

    httplib::Headers headers = { {"X-API-KEY", api_key} };

    auto res = cli.Get(path.c_str(), headers);

    if (res && res->status == 200)
    {
        try {
            auto logs = json::parse(res->body);
            cout << "Found " << logs.size() << " entries:" << endl;
            for (const auto& log : logs)
            {
                cout << " - [" << log["id"] << "] " << log["func"] 
                     << " (RAM: " << log["ram"] << ", Time: " << log["duration"] << ")" << endl;
            }
        } catch (const exception& e) {
             cout << "JSON Parse Error: " << e.what() << endl;
             cout << "Raw Body: " << res->body << endl;
        }
    }
    else
    {
        cout << "Error querying logs. Status: " << (res ? to_string(res->status) : "Connection Failed") << endl;
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        print_help();
        return 1;
    }

    string command = argv[1];

    if (command == "register" && argc == 3)
    {
        register_user(argv[2]);
    }
    else if (command == "log" && argc == 6)
    {
        log_trace(argv[2], argv[3], stoi(argv[4]), stoi(argv[5]));
    }
    else if (command == "query" && argc == 6)
    {
        query_logs(argv[2], argv[3], stoi(argv[4]), stoi(argv[5]));
    }
    else
    {
        print_help();
        return 1;
    }

    return 0;
}