#include "../include/exectrace.hpp"
#include <thread>
#include <chrono>
#include <iostream>
#include <vector>

void heavy_calculation()
{
    TRACE_FUNCTION();

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    std::vector<int> dummy_data(100000, 1);
}

void complex_process()
{
    TRACE_SCOPE("complex_process_v2");

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    heavy_calculation();
}

int main()
{
    std::cout << "Starting Auto-Trace Demo..." << std::endl;

    ExecTrace::init("aiRrV41mtzxlYvKWrO72tK0LK0e1zLOZ");

    complex_process();

    std::cout << "Job Done. Check your logs!" << std::endl;
    return 0;
}