#define CROW_MAIN  
#include <crow.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>
#include "../include/Database.hpp"
#include "../include/AuthDB.hpp"

// Platform-specific includes for directory operations
#ifdef _WIN32
#include <direct.h>   // _getcwd, _mkdir
#include <windows.h>
#else
#include <unistd.h>   // getcwd
#include <sys/stat.h> // mkdir
#endif

// Global database pointers
ExecTraceDB* trace_db = nullptr;
AuthDB* auth_db = nullptr;

// Helper to ensure data directory exists (creates parent directories if needed)
void ensure_directory_exists(const std::string& path) {
#ifdef _WIN32
    struct _stat st;
    if (_stat(path.c_str(), &st) != 0) {
        _mkdir(path.c_str());
        std::cout << "[Server] Created directory: " << path << std::endl;
    }
#else
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        mkdir(path.c_str(), 0755);
        std::cout << "[Server] Created directory: " << path << std::endl;
    }
#endif
}

int main() {
    std::cout << "=== ExecTrace Server (Phase 3.1 - Routing Fixed) ===" << std::endl;
    
    // Print current working directory
#ifdef _WIN32
    char cwd[MAX_PATH];
    if (_getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::cout << "[Server] Current Working Directory: " << cwd << std::endl;
    }
#else
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::cout << "[Server] Current Working Directory: " << cwd << std::endl;
    }
#endif
    
    // Ensure data directory exists (create parent first)
    ensure_directory_exists("backend");
    ensure_directory_exists("backend/data");
    
    // Initialize databases
    try {
        auth_db = new AuthDB("backend/data/users.db", "backend/data/projects.db");
        trace_db = new ExecTraceDB("backend/data/traces.db");
        std::cout << "[Server] Databases initialized" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[Server ERROR] Failed to initialize databases: " << e.what() << std::endl;
        return 1;
    }
    
    // Use SimpleApp (no middleware)
    crow::SimpleApp app;
    
    std::cout << "[Server] Registering routes..." << std::endl;
    
    // Helper to read files
    auto read_file = [](const std::string& path) -> std::string {
        std::ifstream file(path);
        if (!file.is_open()) return "";
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    };
    
    // Frontend routes
    CROW_ROUTE(app, "/")
    ([&read_file](){
        auto content = read_file("frontend/login.html");
        if (content.empty()) {
            return crow::response(404, "File not found");
        }
        crow::response resp(200, content);
        resp.add_header("Content-Type", "text/html");
        return resp;
    });
    
    CROW_ROUTE(app, "/login")
    ([&read_file](){
        auto content = read_file("frontend/login.html");
        if (content.empty()) {
            return crow::response(404, "File not found");
        }
        crow::response resp(200, content);
        resp.add_header("Content-Type", "text/html");
        return resp;
    });
    
    CROW_ROUTE(app, "/dashboard")
    ([&read_file](){
        auto content = read_file("frontend/dashboard.html");
        if (content.empty()) {
            return crow::response(404, "File not found");
        }
        crow::response resp(200, content);
        resp.add_header("Content-Type", "text/html");
        return resp;
    });
    
    CROW_ROUTE(app, "/workspace")
    ([&read_file](){
        auto content = read_file("frontend/workspace.html");
        if (content.empty()) {
            return crow::response(404, "File not found");
        }
        crow::response resp(200, content);
        resp.add_header("Content-Type", "text/html");
        return resp;
    });
    
    
    // Health check endpoint
    CROW_ROUTE(app, "/health")
    ([](){
        std::cout << "[/health] Request received" << std::endl;
        std::cout.flush();
        return "{\"status\":\"ok\",\"database\":\"initialized\"}";
    });
    
    // Log endpoint - POST only
    CROW_ROUTE(app, "/log").methods(crow::HTTPMethod::Post)
    ([&auth_db](const crow::request& req){
        std::cout << "\n[/log] ===== POST REQUEST RECEIVED =====" << std::endl;
        std::cout << "[/log] Content-Type: " << req.get_header_value("Content-Type") << std::endl;
        
        // Extract and validate API Key
        std::string api_key = req.get_header_value("X-API-Key");
        std::cout << "[/log] X-API-Key: " << (api_key.empty() ? "(not provided)" : api_key) << std::endl;
        
        // Get project ID from API key
        int project_id = 1; // Default fallback
        if (!api_key.empty() && auth_db && auth_db->get_project_id_from_api_key(api_key, project_id)) {
            std::cout << "[/log] Validated: Project ID " << project_id << std::endl;
        } else {
            std::cout << "[/log] WARNING: Invalid/missing API key, defaulting to Project 1" << std::endl;
        }
        
        std::cout << "[/log] Body length: " << req.body.length() << std::endl;
        std::cout << "[/log] Body: " << req.body << std::endl;
        std::cout.flush();
        
        try {
            if (!trace_db) {
                std::cerr << "[/log ERROR] Database not initialized" << std::endl;
                return crow::response(500, "{\"error\":\"Database not initialized\"}");
            }
            
            // Manual parsing of URL-encoded POST body
            std::map<std::string, std::string> post_params;
            std::string body = req.body;
            std::istringstream body_stream(body);
            std::string pair;
            while (std::getline(body_stream, pair, '&')) {
                size_t eq_pos = pair.find('=');
                if (eq_pos != std::string::npos) {
                    post_params[pair.substr(0, eq_pos)] = pair.substr(eq_pos + 1);
                }
            }
            
            // Get parameters with defaults
            std::string func = post_params.count("func") ? post_params["func"] : "unknown";
            std::string msg = post_params.count("message") ? post_params["message"] : "Trace";
            std::string version = post_params.count("version") ? post_params["version"] : "v1.0.0";
            
            uint64_t duration = 0;
            uint64_t ram = 0;
            
            if (post_params.count("duration")) {
                try {
                    duration = std::stoull(post_params["duration"]);
                } catch (...) {
                    duration = 0;
                }
            }
            if (post_params.count("ram")) {
                try {
                    ram = std::stoull(post_params["ram"]);
                } catch (...) {
                    ram = 0;
                }
            }
            
            std::cout << "[/log] Parsed params: func=" << func << ", msg=" << msg 
                      << ", version=" << version << ", duration=" << duration 
                      << ", ram=" << ram << std::endl;
            std::cout.flush();
            
            // Log to database with validated project_id
            int log_id = trace_db->log_event(project_id, func.c_str(), msg.c_str(), 
                                            version.c_str(), duration, ram);
            
            std::cout << "[/log] Success! ID=" << log_id << std::endl;
            std::cout.flush();
            
            std::string response_json = "{\"status\":\"ok\",\"id\":" + std::to_string(log_id) + "}";
            
            crow::response resp(200, response_json);
            resp.add_header("Content-Type", "application/json");
            resp.add_header("Access-Control-Allow-Origin", "*");
            return resp;
            
        } catch (const std::exception& e) {
            std::cerr << "[/log ERROR] " << e.what() << std::endl;
            std::cerr.flush();
            
            crow::response resp(500, "{\"error\":\"Server error\",\"details\":\"" + 
                               std::string(e.what()) + "\"}");
            resp.add_header("Content-Type", "application/json");
            resp.add_header("Access-Control-Allow-Origin", "*");
            return resp;
        }
    });
    
    // Register endpoint
    CROW_ROUTE(app, "/api/auth/register").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req){
        std::cout << "\n[/api/auth/register] Request received" << std::endl;
        auto params = crow::query_string(req.body);
        
        std::string email = params.get("email") ? params.get("email") : "";
        std::string password = params.get("password") ? params.get("password") : "";
        std::string username = params.get("username") ? params.get("username") : "";
        
        int user_id;
        if (auth_db->register_user(email, password, username, user_id)) {
            crow::response resp(200, "{\"status\":\"ok\",\"user_id\":" + std::to_string(user_id) + "}");
            resp.add_header("Content-Type", "application/json");
            resp.add_header("Access-Control-Allow-Origin", "*");
            return resp;
        }
        
        crow::response resp(400, "{\"error\":\"User already exists\"}");
        resp.add_header("Content-Type", "application/json");
        return resp;
    });
    
    // Login endpoint
    CROW_ROUTE(app, "/api/auth/login").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req){
        std::cout << "\n[/api/auth/login] Request received" << std::endl;
        auto params = crow::query_string(req.body);
        
        std::string email = params.get("email") ? params.get("email") : "";
        std::string password = params.get("password") ? params.get("password") : "";
        
        ExecTrace::UserEntry user;
        if (auth_db->login_user(email, password, user)) {
            crow::response resp(200, "{\"status\":\"ok\",\"user_id\":" + std::to_string(user.user_id) + ",\"username\":\"" + std::string(user.username) + "\"}");
            resp.add_header("Content-Type", "application/json");
            resp.add_header("Access-Control-Allow-Origin", "*");
            return resp;
        }
        
        crow::response resp(401, "{\"error\":\"Invalid credentials\"}");
        resp.add_header("Content-Type","application/json");
        return resp;
    });
    
    // Create project endpoint
    CROW_ROUTE(app, "/api/project/create").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req){
        std::cout << "\n[/api/project/create] Request received" << std::endl;
        
        // Manual parsing of URL-encoded POST body (crow::query_string doesn't work with POST bodies)
        std::map<std::string, std::string> post_params;
        std::string body = req.body;
        std::istringstream body_stream(body);
        std::string pair;
        while (std::getline(body_stream, pair, '&')) {
            size_t eq_pos = pair.find('=');
            if (eq_pos != std::string::npos) {
                post_params[pair.substr(0, eq_pos)] = pair.substr(eq_pos + 1);
            }
        }
        
        std::string user_id_str = post_params.count("user_id") ? post_params["user_id"] : "0";
        std::string name = post_params.count("name") ? post_params["name"] : "My Project";
        
        int user_id = std::stoi(user_id_str);
        std::string api_key;
        int project_id;
        
        if (auth_db->create_project(user_id, name, api_key, project_id)) {
            std::string json = "{\"status\":\"ok\",\"project_id\":" + std::to_string(project_id) + ",\"api_key\":\"" + api_key + "\"}";
            crow::response resp(200, json);
            resp.add_header("Content-Type", "application/json");
            resp.add_header("Access-Control-Allow-Origin", "*");
            return resp;
        }
        
        crow::response resp(500, "{\"error\":\"Failed to create project\"}");
        resp.add_header("Content-Type", "application/json");
        return resp;
    });
    
    // Get user projects
    CROW_ROUTE(app, "/api/projects/<int>")
    ([&auth_db](int user_id){
        std::cout << "\n[/api/projects] Getting projects for user " << user_id << std::endl;
        
        try {
            auto projects = auth_db->get_projects_by_user(user_id);
            
            // Build JSON array
            std::string json = "{\"status\":\"ok\",\"projects\":[";
            for (size_t i = 0; i < projects.size(); i++) {
                if (i > 0) json += ",";
                json += "{";
                json += "\"id\":" + std::to_string(projects[i].project_id) + ",";
                json += "\"name\":\"" + std::string(projects[i].name) + "\",";
                json += "\"api_key\":\"" + std::string(projects[i].api_key) + "\",";
                json += "\"fast_threshold\":" + std::to_string(projects[i].fast_threshold) + ",";
                json += "\"normal_threshold\":" + std::to_string(projects[i].normal_threshold);
                json += "}";
            }
            json += "]}";
            
            crow::response resp(200, json);
            resp.add_header("Content-Type", "application/json");
            resp.add_header("Access-Control-Allow-Origin", "*");
            return resp;
        } catch (const std::exception& e) {
            crow::response resp(500, "{\"error\":\"Server error\"}");
            resp.add_header("Content-Type", "application/json");
            return resp;
        }
    });
    
    // Update project settings
    CROW_ROUTE(app, "/api/project/<int>/settings").methods(crow::HTTPMethod::Put)
    ([&auth_db](const crow::request& req, int project_id){
        std::cout << "\n[/api/project/settings] Updating settings for project " << project_id << std::endl;
        
        try {
            auto params = crow::query_string(req.body);
            
            int fast_threshold = 100;
            int normal_threshold = 500;
            
            if (params.get("fast_threshold")) {
                fast_threshold = std::stoi(params.get("fast_threshold"));
            }
            if (params.get("normal_threshold")) {
                normal_threshold = std::stoi(params.get("normal_threshold"));
            }
            
            // Update settings in database
            if (auth_db->update_project_settings(project_id, fast_threshold, normal_threshold)) {
                std::cout << "[Settings] Updated successfully" << std::endl;
            }
            
            std::string json = "{\"status\":\"ok\",\"fast_threshold\":" + std::to_string(fast_threshold) +
                             ",\"normal_threshold\":" + std::to_string(normal_threshold) + "}";
            
            crow::response resp(200, json);
            resp.add_header("Content-Type", "application/json");
            resp.add_header("Access-Control-Allow-Origin", "*");
            return resp;
        } catch (const std::exception& e) {
            crow::response resp(500, "{\"error\":\"Server error\"}");
            resp.add_header("Content-Type", "application/json");
            return resp;
        }
    });
    
    // Delete project
    CROW_ROUTE(app, "/api/project/<int>").methods(crow::HTTPMethod::Delete)
    ([&auth_db](int project_id){
        std::cout << "\n[/api/project/delete] Deleting project " << project_id << std::endl;
        
        try {
            if (auth_db->delete_project(project_id)) {
                std::string json = "{\"status\":\"ok\",\"message\":\"Project deleted\"}";
                
                crow::response resp(200, json);
                resp.add_header("Content-Type", "application/json");
                resp.add_header("Access-Control-Allow-Origin", "*");
                return resp;
            } else {
                crow::response resp(404, "{\"error\":\"Project not found\"}");
                resp.add_header("Content-Type", "application/json");
                return resp;
            }
        } catch (const std::exception& e) {
            crow::response resp(500, "{\"error\":\"Server error\"}");
            resp.add_header("Content-Type", "application/json");
            return resp;
        }
    });
    
    // Advanced query endpoint for dashboard
    CROW_ROUTE(app, "/query/advanced")
    ([&trace_db, &auth_db](const crow::request& req){
        std::cout << "\n[/query/advanced] Advanced query request" << std::endl;
        
        auto params = crow::query_string(req.url_params);
        int limit = params.get("limit") ? std::stoi(params.get("limit")) : 50;
        std::string sort_by = params.get("sort_by") ? params.get("sort_by") : "duration";
        std::string sort_order = params.get("sort_order") ? params.get("sort_order") : "desc";
        
        // Get API key from header for per-project filtering
        std::string api_key = req.get_header_value("X-API-Key");
        int project_id = -1;
        
        std::vector<ExecTrace::TraceEntry> results;
        
        if (!api_key.empty() && auth_db && auth_db->get_project_id_from_api_key(api_key, project_id)) {
            std::cout << "[Query] Filtering for project " << project_id << std::endl;
            results = trace_db->search_by_project(project_id);
        } else {
            std::cout << "[Query] No valid API key, returning all traces" << std::endl;
            results = trace_db->get_all_traces();
        }
        
        std::cout << "[Query] Limit: " << limit << ", Sort by: " << sort_by << " (" << sort_order << ")" << std::endl;
        std::cout << "[Query] Found " << results.size() << " traces" << std::endl;
        
        // Sort results based on parameters
        std::sort(results.begin(), results.end(), [&](const ExecTrace::TraceEntry& a, const ExecTrace::TraceEntry& b) {
            bool result = false;
            
            if (sort_by == "duration") {
                result = a.duration < b.duration;
            } else if (sort_by == "ram") {
                result = a.ram_usage < b.ram_usage;
            } else if (sort_by == "func") {
                result = std::string(a.func) < std::string(b.func);
            } else {
                result = a.duration < b.duration; // Default to duration
            }
            
            // Reverse for descending order
            return sort_order == "desc" ? !result : result;
        });
        
        // Build JSON array
        std::string json = "[";
        for (size_t i = 0; i < results.size() && i < (size_t)limit; i++) {
            if (i > 0) json += ",";
            json += "{";
            json += "\"func\":\"" + std::string(results[i].func) + "\",";
            json += "\"message\":\"" + std::string(results[i].message) + "\",";
            json += "\"duration\":" + std::to_string(results[i].duration) + ",";
            json += "\"ram\":" + std::to_string(results[i].ram_usage) + ",";
            json += "\"app_version\":\"" + std::string(results[i].app_version) + "\"";
            json += "}";
        }
        json += "]";
        
        crow::response resp(200, json);
        resp.add_header("Content-Type", "application/json");
        resp.add_header("Access-Control-Allow-Origin", "*");
        return resp;
    });
    
    // Get project info from API key
    CROW_ROUTE(app, "/api/project/info")
    ([](const crow::request& req){
        std::cout << "\n[/api/project/info] Validating API key" << std::endl;
        
        std::string api_key = req.get_header_value("X-API-Key");
        std::cout << "[Info] API Key: " << api_key << std::endl;
        
        // Accept any API key for now
        std::string json = "{\"id\":1,\"name\":\"Demo Project\",\"api_key\":\"" + api_key + "\"}";
        
        crow::response resp(200, json);
        resp.add_header("Content-Type", "application/json");
        resp.add_header("Access-Control-Allow-Origin", "*");
        return resp;
    });
    
    // Get project settings
    CROW_ROUTE(app, "/api/projects/<int>/settings")
    ([](int project_id){
        std::cout << "\n[/api/projects/settings] Project " << project_id << std::endl;
        
        std::string json = "{\"fast_threshold_ms\":100,\"slow_threshold_ms\":500}";
        
        crow::response resp(200, json);
        resp.add_header("Content-Type", "application/json");
        resp.add_header("Access-Control-Allow-Origin", "*");
        return resp;
    });
    
    // Query endpoint - GET logs by project
    CROW_ROUTE(app, "/logs/<int>")
    ([](int project_id){
        std::cout << "\n[/logs] ===== GET REQUEST FOR PROJECT " << project_id << " =====" << std::endl;
        std::cout.flush();
        
        try {
            if (!trace_db) {
                crow::response resp(500, "{\"error\":\"Database not initialized\"}");
                resp.add_header("Content-Type", "application/json");
                return resp;
            }
            
            auto results = trace_db->search_by_project(project_id);
            
            std::cout << "[/logs] Found " << results.size() << " entries" << std::endl;
            
            // Build JSON response manually
            std::string json = "{\"status\":\"ok\",\"count\":" + std::to_string(results.size()) + ",\"logs\":[";
            
            for (size_t i = 0; i < results.size(); i++) {
                const auto& entry = results[i];
                if (i > 0) json += ",";
                
                json += "{";
                json += "\"id\":" + std::to_string(entry.id) + ",";
                json += "\"project_id\":" + std::to_string(entry.project_id) + ",";
                json += "\"func\":\"" + std::string(entry.func) + "\",";
                json += "\"message\":\"" + std::string(entry.message) + "\",";
                json += "\"app_version\":\"" + std::string(entry.app_version) + "\",";
                json += "\"duration\":" + std::to_string(entry.duration) + ",";
                json += "\"ram_usage\":" + std::to_string(entry.ram_usage) + ",";
                json += "\"timestamp\":" + std::to_string(entry.timestamp);
                json += "}";
            }
            
            json += "]}";
            
            crow::response resp(200, json);
            resp.add_header("Content-Type", "application/json");
            resp.add_header("Access-Control-Allow-Origin", "*");
            return resp;
            
        } catch (const std::exception& e) {
            std::cerr << "[/logs ERROR] " << e.what() << std::endl;
            
            crow::response resp(500, "{\"error\":\"Server error\"}");
            resp.add_header("Content-Type", "application/json");
            return resp;
        }
    });
    
    // Debug: List all registered routes
    std::cout << "\n[Server] Routes registered successfully:" << std::endl;
    std::cout << "  GET  /" << std::endl;
    std::cout << "  GET  /health" << std::endl;
    std::cout << "  POST /log" << std::endl;
    std::cout << "  GET  /logs/<int>" << std::endl;
    
    std::cout << "\n[Server] Starting on http://0.0.0.0:8080" << std::endl;
    std::cout << "[Server] Press Ctrl+C to stop\n" << std::endl;
    
    std::cout << "Test commands:" << std::endl;
    std::cout << "  curl http://localhost:8080/" << std::endl;
    std::cout << "  curl http://localhost:8080/health" << std::endl;
    std::cout << "  curl -X POST http://localhost:8080/log -d 'func=test&msg=hello&duration=100&ram=1024'" << std::endl;
    std::cout << "  curl http://localhost:8080/logs/1" << std::endl;
    std::cout << std::endl;
    
    // Start server - single threaded for easier debugging
    app.port(8080).run();
    
    // Cleanup
    if (trace_db) {
        delete trace_db;
    }
    
    return 0;
}