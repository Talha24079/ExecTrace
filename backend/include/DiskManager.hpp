#pragma once
#include "Models.hpp"
#include <fstream>
#include <vector>
#include <mutex>
#include <cstring>
#include <iostream>

const int PAGE_SIZE = 4096;

// DiskManager - handles file I/O for database pages
class DiskManager {
private:
    std::string db_filename;
    std::fstream db_file;
    mutable std::mutex file_mutex;
    int next_page_id;

public:
    DiskManager(const std::string& filename) : db_filename(filename), next_page_id(1) {
        db_file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
        
        if (!db_file.is_open()) {
            // Create new file
            db_file.clear();
            db_file.open(filename, std::ios::out | std::ios::binary);
            db_file.close();
            db_file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
            
            if (!db_file.is_open()) {
                throw std::runtime_error("Failed to create database file: " + filename);
            }
            
            std::cout << "[DiskManager] Created new database: " << filename << std::endl;
        } else {
            // Calculate number of existing pages
            db_file.seekg(0, std::ios::end);
            size_t file_size = db_file.tellg();
            next_page_id = (file_size / PAGE_SIZE) + 1;
            
            std::cout << "[DiskManager] Opened existing database: " << filename 
                      << " (" << (file_size / PAGE_SIZE) << " pages)" << std::endl;
        }
    }

    ~DiskManager() {
        if (db_file.is_open()) {
            db_file.close();
        }
    }

    void write_page(int page_id, const char* data) {
        std::lock_guard<std::mutex> lock(file_mutex);
        db_file.seekp(page_id * PAGE_SIZE, std::ios::beg);
        db_file.write(data, PAGE_SIZE);
        db_file.flush();
    }

    void read_page(int page_id, char* data) {
        std::lock_guard<std::mutex> lock(file_mutex);
        
        // Check if page exists
        db_file.seekg(0, std::ios::end);
        size_t file_size = db_file.tellg();
        
        if (page_id * PAGE_SIZE >= file_size) {
            // Page doesn't exist, return zeroed data
            memset(data, 0, PAGE_SIZE);
            return;
        }
        
        db_file.seekg(page_id * PAGE_SIZE, std::ios::beg);
        db_file.read(data, PAGE_SIZE);
    }

    int allocate_page() {
        std::lock_guard<std::mutex> lock(file_mutex);
        return next_page_id++;
    }
};
