#include "exectrace.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <random>

void fast_function() {
    TRACE_FUNCTION();
    int x = 0;
    for(int i = 0; i < 1000; i++) x += i;
}

void slow_function() {
    TRACE_FUNCTION();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void memory_intensive() {
    TRACE_FUNCTION();
    size_t size = 5 * 1024 * 1024;
    char* buffer = new char[size];
    memset(buffer, 1, size);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    delete[] buffer;
}

int main() {
    std::cout << "ExecTrace Test Seeder" << std::endl;
    std::cout << "Paste your Project API Key: ";
    
    std::string project_key;
    std::getline(std::cin, project_key);
    
    ExecTrace::init(project_key, "v1.0.0", "127.0.0.1", 8080);
    
    std::cout << "Sending test data..." << std::endl;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, 100);

    for (int i = 0; i < 50; i++) {
        int r = dist(gen);
        
        if (r <= 60) {
            fast_function();
        } else if (r <= 80) {
            memory_intensive();
        } else {
            slow_function();
        }
        
        if (i % 10 == 0) {
            std::cout << "Sent " << i << " logs..." << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "✓ Complete! Check your dashboard." << std::endl;
    return 0;
}
