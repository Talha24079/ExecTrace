#include "sdk/exectrace.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <random>
#include <iomanip>
#include <cstring>

// Global variable to control optimization mode
bool optimized = false;

// Matrix structure to handle huge matrices
struct Matrix {
    size_t rows;
    size_t cols;
    double** data;
    
    Matrix(size_t r, size_t c) : rows(r), cols(c) {
        data = new double*[rows];
        for (size_t i = 0; i < rows; i++) {
            data[i] = new double[cols];
        }
    }
    
    ~Matrix() {
        for (size_t i = 0; i < rows; i++) {
            delete[] data[i];
        }
        delete[] data;
    }
    
    // Prevent copying for performance
    Matrix(const Matrix&) = delete;
    Matrix& operator=(const Matrix&) = delete;
};

// Function to initialize matrices with random values
void initialize(Matrix& mat, double min_val = 0.0, double max_val = 100.0) {
    TRACE_FUNCTION();  // ExecTrace tracking
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(min_val, max_val);
    
    std::cout << "Initializing matrix [" << mat.rows << "x" << mat.cols << "]..." << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    if (optimized) {
        // Optimized initialization with cache-friendly access pattern
        for (size_t i = 0; i < mat.rows; i++) {
            for (size_t j = 0; j < mat.cols; j++) {
                mat.data[i][j] = dis(gen);
            }
        }
    } else {
        // Non-optimized initialization
        for (size_t j = 0; j < mat.cols; j++) {
            for (size_t i = 0; i < mat.rows; i++) {
                mat.data[i][j] = dis(gen);
            }
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Initialization completed in " << duration.count() << " ms" 
              << (optimized ? " (optimized)" : " (non-optimized)") << std::endl;
}

// Function to write matrix to file
void file_write(const Matrix& mat, const std::string& filename) {
    TRACE_FUNCTION();  // ExecTrace tracking
    
    std::cout << "Writing matrix to file: " << filename << "..." << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file for writing!" << std::endl;
        return;
    }
    
    // Write matrix dimensions
    file.write(reinterpret_cast<const char*>(&mat.rows), sizeof(size_t));
    file.write(reinterpret_cast<const char*>(&mat.cols), sizeof(size_t));
    
    if (optimized) {
        // Optimized binary write in chunks
        const size_t chunk_size = 1024;
        for (size_t i = 0; i < mat.rows; i++) {
            file.write(reinterpret_cast<const char*>(mat.data[i]), 
                      mat.cols * sizeof(double));
        }
    } else {
        // Non-optimized write (element by element)
        for (size_t i = 0; i < mat.rows; i++) {
            for (size_t j = 0; j < mat.cols; j++) {
                file.write(reinterpret_cast<const char*>(&mat.data[i][j]), 
                          sizeof(double));
            }
        }
    }
    
    file.close();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "File write completed in " << duration.count() << " ms" 
              << (optimized ? " (optimized)" : " (non-optimized)") << std::endl;
}

// Function to perform matrix multiplication
Matrix* multiplication(const Matrix& A, const Matrix& B) {
    TRACE_FUNCTION();  // ExecTrace tracking
    
    if (A.cols != B.rows) {
        std::cerr << "Error: Matrix dimensions incompatible for multiplication!" << std::endl;
        std::cerr << "A: [" << A.rows << "x" << A.cols << "], B: [" << B.rows << "x" << B.cols << "]" << std::endl;
        return nullptr;
    }
    
    std::cout << "Multiplying matrices [" << A.rows << "x" << A.cols << "] * [" 
              << B.rows << "x" << B.cols << "]..." << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    Matrix* result = new Matrix(A.rows, B.cols);
    
    // Initialize result matrix to zero
    for (size_t i = 0; i < result->rows; i++) {
        for (size_t j = 0; j < result->cols; j++) {
            result->data[i][j] = 0.0;
        }
    }
    
    if (optimized) {
        // Optimized multiplication using cache-friendly blocking technique
        const size_t BLOCK_SIZE = 64;
        
        for (size_t ii = 0; ii < A.rows; ii += BLOCK_SIZE) {
            for (size_t jj = 0; jj < B.cols; jj += BLOCK_SIZE) {
                for (size_t kk = 0; kk < A.cols; kk += BLOCK_SIZE) {
                    // Process block
                    for (size_t i = ii; i < std::min(ii + BLOCK_SIZE, A.rows); i++) {
                        for (size_t k = kk; k < std::min(kk + BLOCK_SIZE, A.cols); k++) {
                            double a_ik = A.data[i][k];
                            for (size_t j = jj; j < std::min(jj + BLOCK_SIZE, B.cols); j++) {
                                result->data[i][j] += a_ik * B.data[k][j];
                            }
                        }
                    }
                }
            }
        }
    } else {
        // Non-optimized naive matrix multiplication
        for (size_t i = 0; i < A.rows; i++) {
            for (size_t j = 0; j < B.cols; j++) {
                for (size_t k = 0; k < A.cols; k++) {
                    result->data[i][j] += A.data[i][k] * B.data[k][j];
                }
            }
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Multiplication completed in " << duration.count() << " ms" 
              << (optimized ? " (optimized)" : " (non-optimized)") << std::endl;
    
    return result;
}

// Helper function to print a small portion of the matrix
void print_matrix_sample(const Matrix& mat, size_t sample_size = 5) {
    std::cout << "\nMatrix [" << mat.rows << "x" << mat.cols << "] sample:" << std::endl;
    size_t rows_to_print = std::min(sample_size, mat.rows);
    size_t cols_to_print = std::min(sample_size, mat.cols);
    
    for (size_t i = 0; i < rows_to_print; i++) {
        for (size_t j = 0; j < cols_to_print; j++) {
            std::cout << std::fixed << std::setprecision(2) << std::setw(8) << mat.data[i][j] << " ";
        }
        if (mat.cols > sample_size) {
            std::cout << "...";
        }
        std::cout << std::endl;
    }
    if (mat.rows > sample_size) {
        std::cout << "..." << std::endl;
    }
    std::cout << std::endl;
}

// Helper function to get memory usage of a matrix
size_t get_matrix_memory(const Matrix& mat) {
    return mat.rows * mat.cols * sizeof(double) + mat.rows * sizeof(double*);
}

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "   Huge Matrix Operations Benchmark" << std::endl;
    std::cout << "   with ExecTrace Integration" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Determine version and API key from command line
    std::string version = "v1";  // Default: non-optimized
    std::string api_key = "";
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--optimized" || arg == "v2") {
            optimized = true;
            version = "v2";
        } else if (arg == "--api-key" && i + 1 < argc) {
            api_key = argv[++i];
        } else if (arg.find("--api-key=") == 0) {
            api_key = arg.substr(10);
        }
    }
    
    // Check if API key is provided
    if (api_key.empty()) {
        std::cerr << "Error: API key is required!" << std::endl;
        std::cerr << "Usage: " << argv[0] << " [v1|v2|--optimized] --api-key=<your-api-key>" << std::endl;
        std::cerr << "  v1          : Run non-optimized version (default)" << std::endl;
        std::cerr << "  v2          : Run optimized version" << std::endl;
        std::cerr << "  --optimized : Same as v2" << std::endl;
        return 1;
    }
    
    // Initialize ExecTrace
    ExecTrace::init(api_key, version);
    
    std::cout << "Running in " << (optimized ? "OPTIMIZED (v2)" : "NON-OPTIMIZED (v1)") << " mode" << std::endl;
    std::cout << "Version: " << version << std::endl;
    
    // Define matrix sizes (can be configured)
    const size_t SIZE_A_ROWS = 500;
    const size_t SIZE_A_COLS = 500;
    const size_t SIZE_B_ROWS = 500;
    const size_t SIZE_B_COLS = 500;
    
    std::cout << "\nAllocating matrices..." << std::endl;
    
    // Create huge matrices
    Matrix matrixA(SIZE_A_ROWS, SIZE_A_COLS);
    Matrix matrixB(SIZE_B_ROWS, SIZE_B_COLS);
    
    size_t total_memory = get_matrix_memory(matrixA) + get_matrix_memory(matrixB);
    std::cout << "Total memory allocated: " << (total_memory / (1024.0 * 1024.0)) << " MB" << std::endl;
    
    std::cout << "\n--- Step 1: Initialize Matrices ---" << std::endl;
    initialize(matrixA, 0.0, 10.0);
    initialize(matrixB, 0.0, 10.0);
    
    // Print samples
    print_matrix_sample(matrixA);
    print_matrix_sample(matrixB);
    
    std::cout << "\n--- Step 2: Perform Matrix Multiplication ---" << std::endl;
    Matrix* result = multiplication(matrixA, matrixB);
    
    if (result != nullptr) {
        print_matrix_sample(*result);
        
        total_memory += get_matrix_memory(*result);
        std::cout << "Total memory in use: " << (total_memory / (1024.0 * 1024.0)) << " MB" << std::endl;
        
        std::cout << "\n--- Step 3: Write Result to File ---" << std::endl;
        std::string output_filename = optimized ? "result_optimized.bin" : "result_normal.bin";
        file_write(*result, output_filename);
        
        std::cout << "\n--- Step 4: Write Input Matrices to Files ---" << std::endl;
        file_write(matrixA, "matrix_A.bin");
        file_write(matrixB, "matrix_B.bin");
        
        delete result;
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "   Benchmark Completed Successfully" << std::endl;
    std::cout << "   Check ExecTrace Dashboard for metrics" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Give time for async logs to complete
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    return 0;
}
