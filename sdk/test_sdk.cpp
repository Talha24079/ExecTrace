#include "exectrace.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

using namespace std;

// Test functions with different performance characteristics
void fast_function() {
    TRACE_FUNCTION();
    this_thread::sleep_for(chrono::milliseconds(50));
}

void normal_function() {
    TRACE_FUNCTION();
    this_thread::sleep_for(chrono::milliseconds(250));
}

void slow_function() {
    TRACE_FUNCTION();
    this_thread::sleep_for(chrono::milliseconds(700));
}

void compute_heavy() {
    TRACE_FUNCTION();

    // Some computation
    double result = 0.0;
    for (int i = 0; i < 1000000; i++) {
        result += sqrt(i * 3.14159);
    }

    cout << "Result: " << result << endl;
}

void memory_allocator() {
    TRACE_FUNCTION();

    // Allocate some memory
    vector<int> data(1000000);
    for (int i = 0; i < 1000000; i++) {
        data[i] = i;
    }

    // Keep it alive for a bit
    this_thread::sleep_for(chrono::milliseconds(100));
}

void nested_operations() {
    TRACE_SCOPE("nested_operations");

    cout << "Starting nested operations..." << endl;

    {
        TRACE_SCOPE("nested_operations::phase1");
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    {
        TRACE_SCOPE("nested_operations::phase2");
        this_thread::sleep_for(chrono::milliseconds(150));
    }

    {
        TRACE_SCOPE("nested_operations::phase3");
        this_thread::sleep_for(chrono::milliseconds(75));
    }
}

int main() {
    cout << "==================================" << endl;
    cout << "ExecTrace SDK Test Program" << endl;
    cout << "==================================" << endl << endl;

    string api_key = "your-api-key";

    cout << "Configuration:" << endl;
    cout << "  API Key: " << api_key << endl;
    cout << "  Version: " << ExecTrace::auto_version() << endl;
    cout << "  Server: localhost:9090" << endl;
    cout << endl;

    // Initialize ExecTrace
    cout << "Initializing ExecTrace SDK..." << endl;
    ExecTrace::init(api_key, ExecTrace::auto_version(), "127.0.0.1", 9090);
    cout << endl;

    // Wait a moment for connection to establish
    this_thread::sleep_for(chrono::seconds(1));

    cout << "==================================" << endl;
    cout << "Running Tests..." << endl;
    cout << "==================================" << endl << endl;

    // Test 1: Fast function
    cout << "[Test 1] Fast function (should be <100ms)..." << endl;
    fast_function();
    cout << "✓ Complete" << endl << endl;

    // Test 2: Normal function
    cout << "[Test 2] Normal function (should be 100-500ms)..." << endl;
    normal_function();
    cout << "✓ Complete" << endl << endl;

    // Test 3: Slow function
    cout << "[Test 3] Slow function (should be >500ms)..." << endl;
    slow_function();
    cout << "✓ Complete" << endl << endl;

    // Test 4: Compute heavy
    cout << "[Test 4] Compute-heavy function..." << endl;
    compute_heavy();
    cout << "✓ Complete" << endl << endl;

    // Test 5: Memory allocator
    cout << "[Test 5] Memory allocation test..." << endl;
    memory_allocator();
    cout << "✓ Complete" << endl << endl;

    // Test 6: Nested operations
    cout << "[Test 6] Nested operations..." << endl;
    nested_operations();
    cout << "✓ Complete" << endl << endl;

    // Test 7: Multiple rapid calls
    cout << "[Test 7] Rapid-fire logging (10 calls)..." << endl;
    for (int i = 0; i < 10; i++) {
        TRACE_SCOPE("rapid_call_" + to_string(i));
        this_thread::sleep_for(chrono::milliseconds(20));
    }
    cout << "✓ Complete" << endl << endl;

    // Test 8: Manual logging
    cout << "[Test 8] Manual logging test..." << endl;
    auto start = chrono::high_resolution_clock::now();
    this_thread::sleep_for(chrono::milliseconds(300));
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    ExecTrace::log("manual_test", duration, ExecTrace::get_current_ram_kb(), "Custom message");
    cout << "✓ Complete" << endl << endl;

    cout << "==================================" << endl;
    cout << "All Tests Complete!" << endl;
    cout << "==================================" << endl << endl;

    cout << "View your results at:" << endl;
    cout << "  http://localhost:9090/dashboard?key=" << api_key << endl << endl;

    cout << "Waiting for traces to send..." << endl;
    this_thread::sleep_for(chrono::seconds(3));
    
    cout << "✓ Done! Check your dashboard." << endl;

    return 0;
}
