#define CROW_MAIN  
#include <crow.h>
#include <iostream>
#include "../include/Database.hpp"
#include "../include/AuthDB.hpp"
#include "../include/Models.hpp"
#include "../include/Utils.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>

#ifdef _WIN32
#include <direct.h>   
#include <windows.h>
#else
#include <unistd.h>   
#include <sys/stat.h> 
#endif

ExecTraceDB* trace_db = nullptr;
AuthDB* auth_db = nullptr;

RateLimiter rate_limiter;

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

    ensure_directory_exists("backend");
    ensure_directory_exists("backend/data");

    init_logger("backend/data/server.log");
    log_info("Server", "Starting ExecTrace server...");

    try {
        auth_db = new AuthDB("backend/data/users.db", "backend/data/projects.db");
        trace_db = new ExecTraceDB("backend/data/traces.db");
        std::cout << "[Server] Databases initialized" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[Server ERROR] Failed to initialize databases: " << e.what() << std::endl;
        return 1;
    }

    crow::SimpleApp app;
    
    std::cout << "[Server] Registering routes..." << std::endl;

    auto read_file = [](const std::string& path) -> std::string {
        std::ifstream file(path);
        if (!file.is_open()) return "";
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    };

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
    
    CROW_ROUTE(app, "/admin")
    ([&read_file](){
        auto content = read_file("frontend/admin.html");
        if (content.empty()) {
            return crow::response(404, "File not found");
        }
        crow::response resp(200, content);
        resp.add_header("Content-Type", "text/html");
        return resp;
    });
    
    CROW_ROUTE(app, "/analytics")
    ([&read_file](){
        auto content = read_file("frontend/analytics.html");
        if (content.empty()) {
            return crow::response(404, "File not found");
        }
        crow::response resp(200, content);
        resp.add_header("Content-Type", "text/html");
        return resp;
    });

    CROW_ROUTE(app, "/health")
    ([](){
        std::cout << "[/health] Request received" << std::endl;
        std::cout.flush();
        return "{\"status\":\"ok\",\"database\":\"initialized\"}";
    });

    CROW_ROUTE(app, "/log").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req){
        std::cout << "\n[/log] ===== POST REQUEST RECEIVED =====" << std::endl;
        std::cout << "[/log] Content-Type: " << req.get_header_value("Content-Type") << std::endl;

        std::string api_key = req.get_header_value("X-API-Key");
        std::cout << "[/log] X-API-Key: " << (api_key.empty() ? "(not provided)" : api_key) << std::endl;

        if (!rate_limiter.allow_request(api_key)) {
            log_warn("RateLimit", "Rate limit exceeded for: " + api_key.substr(0, 12));
            crow::response resp(429, "{\"error\":\"Rate limit exceeded. Please slow down.\"}");
            resp.add_header("Content-Type", "application/json");
            resp.add_header("Retry-After", "60");
            return resp;
        }

        int project_id = 1; 
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

            func = ExecTrace::sanitize_string(func, 128);
            msg = ExecTrace::sanitize_string(msg, 256);
            version = ExecTrace::sanitize_string(version, 32);
            
            if (!ExecTrace::validate_duration(duration)) {
                log_warn("Validation", "Invalid duration: " + std::to_string(duration));
                crow::response resp(400, "{\"error\":\"Invalid duration (must be < 1 hour in ms)\"}");
                resp.add_header("Content-Type", "application/json");
                return resp;
            }
            
            if (!ExecTrace::validate_ram(ram)) {
                log_warn("Validation", "Invalid RAM: " + std::to_string(ram));
                crow::response resp(400, "{\"error\":\"Invalid RAM (must be < 100GB in KB)\"}");
                resp.add_header("Content-Type", "application/json");
                return resp;
            }
            
            std::cout << "[/log] Validation passed" << std::endl;
            std::cout.flush();

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

    CROW_ROUTE(app, "/api/auth/register").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req){
        std::cout << "\n[/api/auth/register] Request received" << std::endl;
        std::cout << "[DEBUG] Request body: " << req.body << std::endl;

        auto url_decode = [](const std::string& str) -> std::string {
            std::string result;
            for (size_t i = 0; i < str.length(); i++) {
                if (str[i] == '%' && i + 2 < str.length()) {
                    int value;
                    std::istringstream is(str.substr(i + 1, 2));
                    if (is >> std::hex >> value) {
                        result += static_cast<char>(value);
                        i += 2;
                    } else {
                        result += str[i];
                    }
                } else if (str[i] == '+') {
                    result += ' ';
                } else {
                    result += str[i];
                }
            }
            return result;
        };

        std::string body = req.body;
        std::string email, password, username;

        size_t email_pos = body.find("email=");
        if (email_pos != std::string::npos) {
            size_t email_end = body.find("&", email_pos);
            std::string email_encoded = body.substr(email_pos + 6, 
                email_end == std::string::npos ? std::string::npos : email_end - email_pos - 6);
            email = url_decode(email_encoded);
        }

        size_t pwd_pos = body.find("password=");
        if (pwd_pos != std::string::npos) {
            size_t pwd_end = body.find("&", pwd_pos);
            std::string pwd_encoded = body.substr(pwd_pos + 9, 
                pwd_end == std::string::npos ? std::string::npos : pwd_end - pwd_pos - 9);
            password = url_decode(pwd_encoded);
        }

        size_t user_pos = body.find("username=");
        if (user_pos != std::string::npos) {
            size_t user_end = body.find("&", user_pos);
            std::string user_encoded = body.substr(user_pos + 9, 
                user_end == std::string::npos ? std::string::npos : user_end - user_pos - 9);
            username = url_decode(user_encoded);
        }
        
        std::cout << "[DEBUG] Email: '" << email << "'" << std::endl;
        std::cout << "[DEBUG] Username: '" << username << "'" << std::endl;
        std::cout << "[DEBUG] Password length: " << password.length() << std::endl;
        
        if (email.empty() || password.empty() || username.empty()) {
            crow::response resp(400, "{\"error\":\"Missing required fields\"}");
            resp.add_header("Content-Type", "application/json");
            return resp;
        }
        
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

    CROW_ROUTE(app, "/api/auth/login").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req){
        std::cout << "\n[/api/auth/login] Request received" << std::endl;
        std::cout << "[DEBUG] Request body: " << req.body << std::endl;

        auto url_decode = [](const std::string& str) -> std::string {
            std::string result;
            for (size_t i = 0; i < str.length(); i++) {
                if (str[i] == '%' && i + 2 < str.length()) {
                    int value;
                    std::istringstream is(str.substr(i + 1, 2));
                    if (is >> std::hex >> value) {
                        result += static_cast<char>(value);
                        i += 2;
                    } else {
                        result += str[i];
                    }
                } else if (str[i] == '+') {
                    result += ' ';
                } else {
                    result += str[i];
                }
            }
            return result;
        };

        std::string body = req.body;
        std::string email, password;

        size_t email_pos = body.find("email=");
        if (email_pos != std::string::npos) {
            size_t email_end = body.find("&", email_pos);
            std::string email_encoded = body.substr(email_pos + 6, 
                email_end == std::string::npos ? std::string::npos : email_end - email_pos - 6);
            email = url_decode(email_encoded);
        }

        size_t pwd_pos = body.find("password=");
        if (pwd_pos != std::string::npos) {
            size_t pwd_end = body.find("&", pwd_pos);
            std::string pwd_encoded = body.substr(pwd_pos + 9, 
                pwd_end == std::string::npos ? std::string::npos : pwd_end - pwd_pos - 9);
            password = url_decode(pwd_encoded);
        }
        
        std::cout << "[DEBUG] Email: '" << email << "'" << std::endl;
        std::cout << "[DEBUG] Password length: " << password.length() << std::endl;
        
        ExecTrace::UserEntry user;
        if (auth_db->login_user(email, password, user)) {
            
            std::string json_resp = "{\"status\":\"ok\",\"user_id\":" + std::to_string(user.user_id) + 
                                   ",\"username\":\"" + std::string(user.username) + 
                                   "\",\"role\":" + std::to_string(user.role) + "}";
            crow::response resp(200, json_resp);
            resp.add_header("Content-Type", "application/json");
            resp.add_header("Access-Control-Allow-Origin", "*");
            return resp;
        }
        
        crow::response resp(401, "{\"error\":\"Invalid credentials\"}");
        resp.add_header("Content-Type","application/json");
        return resp;
    });

    CROW_ROUTE(app, "/api/project/create").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req){
        std::cout << "\n[/api/project/create] Request received" << std::endl;

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

    CROW_ROUTE(app, "/api/projects/<int>")
    ([](int user_id){
        std::cout << "\n[/api/projects] Getting projects for user " << user_id << std::endl;
        
        try {
            auto projects = auth_db->get_projects_by_user(user_id);

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

    CROW_ROUTE(app, "/api/project/<int>/settings").methods(crow::HTTPMethod::Put)
    ([](const crow::request& req, int project_id){
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

    CROW_ROUTE(app, "/api/project/<int>").methods(crow::HTTPMethod::Delete)
    ([](int project_id){
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

    CROW_ROUTE(app, "/api/admin/users").methods(crow::HTTPMethod::Get)
    ([](const crow::request& req){
        std::cout << "\n[/api/admin/users] Get all users request" << std::endl;

        auto params = crow::query_string(req.url_params);
        std::string user_id_str = params.get("admin_user_id") ? params.get("admin_user_id") : "";
        
        if (user_id_str.empty()) {
            crow::response resp(401, "{\"error\":\"Authentication required\"}");
            resp.add_header("Content-Type", "application/json");
            return resp;
        }
        
        int admin_user_id = std::stoi(user_id_str);

        if (!auth_db->has_permission(admin_user_id, "manage_users")) {
            crow::response resp(403, "{\"error\":\"Admin access required\"}");
            resp.add_header("Content-Type", "application/json");
            return resp;
        }
        
        try {
            auto users = auth_db->get_all_users();

            std::string json = "{\"status\":\"ok\",\"users\":[";
            for (size_t i = 0; i < users.size(); i++) {
                if (i > 0) json += ",";
                json += "{";
                json += "\"user_id\":" + std::to_string(users[i].user_id) + ",";
                json += "\"email\":\"" + std::string(users[i].email) + "\",";
                json += "\"username\":\"" + std::string(users[i].username) + "\",";
                json += "\"role\":" + std::to_string(users[i].role) + ",";
                json += "\"is_active\":" + std::string(users[i].is_active ? "true" : "false") + ",";
                json += "\"created_at\":" + std::to_string(users[i].created_at);
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

    CROW_ROUTE(app, "/api/admin/users/<int>/role").methods(crow::HTTPMethod::Put)
    ([](const crow::request& req, int target_user_id){
        std::cout << "\n[/api/admin/users/role] Update role for user " << target_user_id << std::endl;
        std::cout << "[DEBUG] Request body: " << req.body << std::endl;

        std::string body = req.body;
        std::string admin_user_id_str, new_role_str;

        size_t admin_pos = body.find("admin_user_id=");
        if (admin_pos != std::string::npos) {
            size_t admin_end = body.find("&", admin_pos);
            admin_user_id_str = body.substr(admin_pos + 14, 
                admin_end == std::string::npos ? std::string::npos : admin_end - admin_pos - 14);
        }

        size_t role_pos = body.find("role=");
        if (role_pos != std::string::npos) {
            size_t role_end = body.find("&", role_pos);
            new_role_str = body.substr(role_pos + 5, 
                role_end == std::string::npos ? std::string::npos : role_end - role_pos - 5);
        }
        
        std::cout << "[DEBUG] admin_user_id: '" << admin_user_id_str << "'" << std::endl;
        std::cout << "[DEBUG] role: '" << new_role_str << "'" << std::endl;
        
        if (admin_user_id_str.empty() || new_role_str.empty()) {
            crow::response resp(400, "{\"error\":\"Missing parameters\"}");
            resp.add_header("Content-Type", "application/json");
            return resp;
        }
        
        int admin_user_id = std::stoi(admin_user_id_str);
        int new_role = std::stoi(new_role_str);

        if (new_role < 0 || new_role > 2) {
            crow::response resp(400, "{\"error\":\"Invalid role value. Must be 0 (User), 1 (Editor), or 2 (Admin)\"}");
            resp.add_header("Content-Type", "application/json");
            return resp;
        }

        if (!auth_db->has_permission(admin_user_id, "assign_roles")) {
            crow::response resp(403, "{\"error\":\"Admin access required\"}");
            resp.add_header("Content-Type", "application/json");
            return resp;
        }
        
        try {
            if (auth_db->update_user_role(target_user_id, new_role)) {
                const char* role_names[] = {"User", "Editor", "Admin"};
                std::string json = "{\"status\":\"ok\",\"message\":\"Role updated to " + 
                                  std::string(role_names[new_role]) + "\"}";
                
                crow::response resp(200, json);
                resp.add_header("Content-Type", "application/json");
                resp.add_header("Access-Control-Allow-Origin", "*");
                return resp;
            } else {
                crow::response resp(400, "{\"error\":\"Failed to update role\"}");
                resp.add_header("Content-Type", "application/json");
                return resp;
            }
        } catch (const std::exception& e) {
            crow::response resp(500, "{\"error\":\"Server error\"}");
            resp.add_header("Content-Type", "application/json");
            return resp;
        }
    });

    CROW_ROUTE(app, "/api/admin/users/<int>").methods(crow::HTTPMethod::Delete)
    ([](const crow::request& req, int target_user_id){
        std::cout << "\n[/api/admin/users/delete] Deactivate user " << target_user_id << std::endl;
        
        auto params = crow::query_string(req.url_params);
        std::string admin_user_id_str = params.get("admin_user_id") ? params.get("admin_user_id") : "";
        
        if (admin_user_id_str.empty()) {
            crow::response resp(401, "{\"error\":\"Authentication required\"}");
            resp.add_header("Content-Type", "application/json");
            return resp;
        }
        
        int admin_user_id = std::stoi(admin_user_id_str);

        if (!auth_db->has_permission(admin_user_id, "manage_users")) {
            crow::response resp(403, "{\"error\":\"Admin access required\"}");
            resp.add_header("Content-Type", "application/json");
            return resp;
        }
        
        try {
            if (auth_db->deactivate_user(target_user_id)) {
                crow::response resp(200, "{\"status\":\"ok\",\"message\":\"User deactivated\"}");
                resp.add_header("Content-Type", "application/json");
                resp.add_header("Access-Control-Allow-Origin", "*");
                return resp;
            } else {
                crow::response resp(400, "{\"error\":\"Failed to deactivate user\"}");
                resp.add_header("Content-Type", "application/json");
                return resp;
            }
        } catch (const std::exception& e) {
            crow::response resp(500, "{\"error\":\"Server error\"}");
            resp.add_header("Content-Type", "application/json");
            return resp;
        }
    });

    CROW_ROUTE(app, "/query/advanced")
    ([](const crow::request& req){
        std::cout << "\n[/query/advanced] Advanced query request" << std::endl;
        
        auto params = crow::query_string(req.url_params);
        int limit = params.get("limit") ? std::stoi(params.get("limit")) : 50;
        std::string sort_by = params.get("sort_by") ? params.get("sort_by") : "duration";
        std::string sort_order = params.get("sort_order") ? params.get("sort_order") : "desc";

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

        std::sort(results.begin(), results.end(), [&](const ExecTrace::TraceEntry& a, const ExecTrace::TraceEntry& b) {
            bool result = false;
            
            if (sort_by == "duration") {
                result = a.duration < b.duration;
            } else if (sort_by == "ram") {
                result = a.ram_usage < b.ram_usage;
            } else if (sort_by == "func") {
                result = std::string(a.func) < std::string(b.func);
            } else {
                result = a.duration < b.duration; 
            }

            return sort_order == "desc" ? !result : result;
        });

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

    CROW_ROUTE(app, "/api/project/info")
    ([](const crow::request& req){
        std::cout << "\n[/api/project/info] Validating API key" << std::endl;
        
        std::string api_key = req.get_header_value("X-API-Key");
        std::cout << "[Info] API Key: " << api_key << std::endl;

        std::string json = "{\"id\":1,\"name\":\"Demo Project\",\"api_key\":\"" + api_key + "\"}";
        
        crow::response resp(200, json);
        resp.add_header("Content-Type", "application/json");
        resp.add_header("Access-Control-Allow-Origin", "*");
        return resp;
    });

    CROW_ROUTE(app, "/api/projects/<int>/settings")
    ([](int project_id){
        std::cout << "\n[/api/projects/settings] Project " << project_id << std::endl;
        
        std::string json = "{\"fast_threshold_ms\":100,\"slow_threshold_ms\":500}";
        
        crow::response resp(200, json);
        resp.add_header("Content-Type", "application/json");
        resp.add_header("Access-Control-Allow-Origin", "*");
        return resp;
    });

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

    CROW_ROUTE(app, "/api/stats/<int>")
    ([](const crow::request& req, int project_id){
        try {
            auto all_traces = trace_db->search_by_project(project_id);
            std::vector<ExecTrace::TraceEntry> active;
            for (const auto& t : all_traces) {
                if (!t.is_deleted) active.push_back(t);
            }
            
            if (active.empty()) {
                crow::response resp("{\"total\":0}");
                resp.add_header("Content-Type", "application/json");
                resp.add_header("Access-Control-Allow-Origin", "*");
                return resp;
            }
            
            uint64_t total_duration = 0, min_duration = UINT64_MAX, max_duration = 0;
            uint64_t total_ram = 0;
            for (const auto& t : active) {
                total_duration += t.duration;
                total_ram += t.ram_usage;
                min_duration = std::min(min_duration, t.duration);
                max_duration = std::max(max_duration, t.duration);
            }
            
            uint64_t avg_duration = total_duration / active.size();
            uint64_t avg_ram = total_ram / active.size();
            
            std::stringstream json;
            json << "{\"total\":" << active.size() << ","
                 << "\"duration\":{\"avg\":" << avg_duration << ",\"min\":" << min_duration << ",\"max\":" << max_duration << "},"
                 << "\"ram\":{\"avg\":" << avg_ram << "}}";
            
            crow::response resp(json.str());
            resp.add_header("Content-Type", "application/json");
            resp.add_header("Access-Control-Allow-Origin", "*");
            return resp;
        } catch (const std::exception& e) {
            return crow::response(500, "{\"error\":\"Internal server error\"}");
        }
    });
    
    log_info("Server", "Starting on port 8080...");
    
    app.port(8080).run();

    if (trace_db) {
        delete trace_db;
    }
    
    return 0;
}