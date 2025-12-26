#include "exectrace.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include <cmath>

const bool USE_OPTIMIZED_FILTER = false;
const bool USE_SLOW_DISK = false;

void apply_filter(int image_id) {
    TRACE_FUNCTION();
    std::cout << "  [Filter] Processing Image " << image_id << "..." << std::endl;
    if (USE_OPTIMIZED_FILTER) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    else std::this_thread::sleep_for(std::chrono::milliseconds(300));
}

void load_image_batch() {
    TRACE_FUNCTION();
    if (USE_SLOW_DISK) {
        std::cout << "  [Disk] Loading batch (Deep Verification)..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    else {
        std::cout << "  [Disk] Loading batch..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

void save_results() {
    TRACE_FUNCTION();
    std::cout << "  [Disk] Saving results..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

int main() {
    // Initialize with explicit version for proper dashboard sorting
    // You can use "1", "2", "3" or "v1.0", "v1.1", etc.
    ExecTrace::init("your-api-key", "1.0");


    std::cout << "=== Starting Batch Image Processor ===" << std::endl;

    for (int i = 1; i <= 5; i++) {
        TRACE_SCOPE("main_loop_iteration");

        load_image_batch();

        {
            TRACE_SCOPE("allocation_spike");
            std::vector<int> heavy_buffer(1000000, 1);
            apply_filter(i);
        }

        save_results();
        std::cout << "-----------------------------------" << std::endl;
    }

    std::cout << "=== Batch Complete. Sending final logs... ===" << std::endl;

    // FIX: Wait for background threads to finish sending logs
    std::this_thread::sleep_for(std::chrono::seconds(3));

    return 0;
}