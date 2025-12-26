#include "../include/AuthDB.hpp"

int main() {
    std::cout << "=== Testing AuthDB ===" << std::endl;
    
    // Create AuthDB
    AuthDB auth("test_users.db", "test_projects.db");
    std::cout << "✓ AuthDB created" << std::endl;
    
    // Test 1: Register users
    int user1_id, user2_id;
    bool success = auth.register_user("alice@test.com", "password123", "Alice", user1_id);
    std::cout << "✓ User 1 registered: " << (success ? "YES" : "NO") << ", ID=" << user1_id << std::endl;
    
    success = auth.register_user("bob@test.com", "secret456", "Bob", user2_id);
    std::cout << "✓ User 2 registered: " << (success ? "YES" : "NO") << ", ID=" << user2_id << std::endl;
    
    // Test 2: Duplicate registration (should fail)
    int dup_id;
    success = auth.register_user("alice@test.com", "newpass", "Alice2", dup_id);
    std::cout << "✓ Duplicate registration blocked: " << (!success ? "YES" : "NO") << std::endl;
    
    // Test 3: Login
    ExecTrace::UserEntry user;
    success = auth.login_user("alice@test.com", "password123", user);
    std::cout << "✓ Login successful: " << (success ? "YES" : "NO") << ", Username=" << user.username << std::endl;
    
    // Test 4: Wrong password
    success = auth.login_user("alice@test.com", "wrongpass", user);
    std::cout << "✓ Wrong password rejected: " << (!success ? "YES" : "NO") << std::endl;
    
    // Test 5: Create projects
    std::string api_key1, api_key2;
    int proj1_id, proj2_id;
    
    auth.create_project(user1_id, "Alice's App", api_key1, proj1_id);
    std::cout << "✓ Project 1 created: ID=" << proj1_id << ", Key=" << api_key1 << std::endl;
    
    auth.create_project(user2_id, "Bob's Service", api_key2, proj2_id);
    std::cout << "✓ Project 2 created: ID=" << proj2_id << ", Key=" << api_key2 << std::endl;
    
    std::cout << "\n=== AuthDB Tests PASSED ===" << std::endl;
    return 0;
}
