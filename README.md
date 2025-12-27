# ExecTrace

> A lightweight, cross-platform C++ profiler for real-time function tracing and performance monitoring.

[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-blue.svg)](https://github.com)
[![Language](https://img.shields.io/badge/language-C%2B%2B17-orange.svg)](https://github.com)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
- [Project Structure](#project-structure)
- [Key Components](#key-components)
- [Quick Start](#quick-start)
- [Installation](#installation)
- [Usage](#usage)
- [API Documentation](#api-documentation)
- [Building from Source](#building-from-source)
- [Docker Deployment](#docker-deployment)
- [Contributing](#contributing)

## 🎯 Overview

**ExecTrace** is a modern C++ profiling system that enables developers to track function execution times, memory usage, and application performance in real-time. With automatic RAII-based tracing and a powerful web dashboard, ExecTrace makes performance monitoring effortless.

### Why ExecTrace?

- 🚀 **Zero-Overhead Integration**: Single-header SDK with minimal performance impact
- 🔄 **Real-Time Monitoring**: Live dashboard for instant performance insights
- 🌐 **Cross-Platform**: Works seamlessly on Windows and Linux
- 📊 **Rich Analytics**: Track execution time, memory usage, and function call patterns
- 🎨 **Beautiful Dashboard**: Modern web UI for visualization and analysis
- 🔧 **Easy Setup**: Drop-in SDK with no external dependencies required

## ✨ Features

### SDK Features
- **Automatic Function Tracing**: RAII-based scope tracers with `TRACE_FUNCTION()` and `TRACE_SCOPE(name)`
- **Manual Logging**: Explicit logging with custom metrics
- **Cross-Platform Support**: Windows and Linux with platform-specific optimizations
- **Async Communication**: Non-blocking background requests to server
- **Memory Tracking**: Automatic RAM usage monitoring per function call

### Server Features
- **RESTful API**: Clean HTTP endpoints for logging and querying
- **SQLite Storage**: Lightweight, embedded database for trace persistence
- **Multi-Project Support**: Organize traces by project with API key authentication
- **Real-Time Queries**: Fast retrieval and filtering of trace data
- **User Management**: Authentication and project ownership

### Dashboard Features
- **Live Metrics**: Real-time visualization of function performance
- **Project Workspace**: Manage multiple projects with individual settings
- **Performance Thresholds**: Customizable fast/normal/slow categorization
- **Search & Filter**: Advanced query capabilities
- **Responsive Design**: Works on desktop and mobile browsers

## 🏗️ Architecture

### Backend (C++)
The backend is a high-performance C++ server built with:
- **Server Framework:** Crow (C++ Microframework)
- **Database:** Custom B-Tree implementation persisted to disk
- **Concurrency:** Thread-safe operations using `std::mutex` and persistent locks
- **Architecture:** Monolithic server handling both API requests and static file serving

### Frontend (HTML/CSS/JS)
The frontend is a lightweight, dependency-free Single Page Application (SPA) feel:
- **Tech Stack:** Vanilla HTML5, CSS3 (Inter font), and JavaScript (ES6+)
- **Design:** Modern glassmorphism UI with responsive layout
- **Communication:** Fetch API for REST communication with backend

### Communication Flow

```
[Your C++ App] ──(1)──> [ExecTrace SDK] ──(HTTP/curl)──> [Server:8080] ──> [B-Tree DB]
                                                                ↓
                                                         [Dashboard UI]
```

1. **Client Side**: Your app uses SDK macros/functions to trace execution
2. **Transport**: SDK sends trace data via HTTP POST with API key
3. **Server Side**: Crow-based server receives, parses, and stores traces
4. **Visualization**: Web dashboard queries server for analytics

## 📂 Project Structure

```
ExecTrace/
├── backend/
│   ├── src/
│   │   └── server.cpp       # Main server entry point & API routes
│   ├── include/
│   │   ├── AuthDB.hpp       # User & Project management (uses BTree)
│   │   ├── BTree.hpp        # Core database data structure
│   │   ├── Database.hpp     # Trace storage logic
│   │   ├── DiskManager.hpp  # Low-level disk I/O
│   │   ├── Models.hpp       # Data structures (User, Project, Trace)
│   │   └── Utils.hpp        # Utilities (Validation, RateLimiter, Logger)
│   └── data/                # Persistent database files (*.db)
├── frontend/
│   ├── login.html           # Authentication page
│   ├── workspace.html       # Main user dashboard (Project Management)
│   ├── dashboard.html       # Individual project visualization
│   └── admin.html           # Admin capabilities (User Management)
└── sdk/                     # Client SDKs for integration
```

## 🔑 Key Components

### Authentication & RBAC
- **Users:** Managed via `AuthDB`. Password hashing uses simple SHA-256.
- **Roles:**
  - `Admin` (ID=1): Full access, can manage users.
  - `Editor`: Can edit projects.
  - `User`: Standard access.
- **Safety:** The database automatically rebuilds itself on role changes to prevent duplication bugs.

### Database (B-Tree)
- Custom disk-based B-Tree implementation (`BTree.hpp`).
- Stores `UserEntry`, `ProjectEntry`, and `TraceEntry` structs.
- Supports high-performance searching by hash or ID.

### Utilities (`Utils.hpp`)
- **Validation:** Input sanitization for security (XSS prevention, SQLi prevention).
- **RateLimiter:** Token bucket algorithm to limit API requests.
- **Logger:** Thread-safe logging to file and console.

## 🚀 Quick Start

### 1. Start the Server (Docker)

```bash
git clone https://github.com/yourusername/ExecTrace.git
cd ExecTrace
docker-compose up -d
```

The server will be available at `http://localhost:9090`

### 2. Integrate SDK in Your C++ Project

```cpp
#include "exectrace.hpp"

int main() {
    // Initialize with your API key
    ExecTrace::init("your-api-key", "v1.0.0", "127.0.0.1", 9090);
    
    // Automatic function tracing
    TRACE_FUNCTION();
    
    // Your code here
    processData();
    
    return 0;
}

void processData() {
    TRACE_SCOPE("processData");
    
    // This function's execution time and memory usage
    // will be automatically logged
    for (int i = 0; i < 1000000; i++) {
        // Heavy computation
    }
}
```

### 3. View Results

Navigate to `http://localhost:9090` to see your traces in real-time!

## 📦 Installation

### Prerequisites

**For SDK (Client)**:
- C++17 compatible compiler (GCC 7+, MSVC 2017+, Clang 5+)
- curl command-line tool (for HTTP requests)

**For Server**:
- C++17 compiler
- Boost libraries (1.70+)
- Crow web framework (included)
- SQLite3 (auto-included)

**For Docker Deployment**:
- Docker & Docker Compose

## 💻 Usage

### SDK Integration

#### Automatic Function Tracing

```cpp
#include "exectrace.hpp"

void myFunction() {
    TRACE_FUNCTION();  // Automatically uses function name
    
    // Function body
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
```

#### Custom Scope Tracing

```cpp
void complexOperation() {
    {
        TRACE_SCOPE("Database Query");
        // Database operations
    }
    
    {
        TRACE_SCOPE("Image Processing");
        // Image processing
    }
}
```

#### Manual Logging

```cpp
// Measure a custom operation
auto start = std::chrono::high_resolution_clock::now();

performOperation();

auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
long ram_kb = ExecTrace::get_current_ram_kb();

ExecTrace::log("CustomOperation", duration, ram_kb, "Custom metric");
```

## 📚 API Documentation

### SDK API

#### Initialization

```cpp
void ExecTrace::init(
    const std::string& api_key,
    const std::string& version = "v1.0.0",
    const std::string& host = "127.0.0.1",
    int port = 9090
);
```

#### Manual Logging

```cpp
void ExecTrace::log(
    const std::string& func_name,
    long duration_ms,
    long ram_kb,
    const std::string& message = "Manual trace"
);
```

### REST API Endpoints

#### Auth
- `POST /api/auth/register` - Create account
- `POST /api/auth/login` - Login

#### Projects
- `POST /api/projects` - Create project
- `GET /api/projects` - List user projects
- `POST /api/projects/:id/settings` - Update thresholds

#### Admin
- `GET /api/admin/users` - List all users
- `POST /api/admin/users/:id/role` - Update user role
- `POST /api/admin/users/:id/deactivate` - Deactivate user

#### Tracing
- `POST /api/trace` - Ingest performance data (`func`, `message`, `duration`, `ram`, `version`)

## 🔨 Building from Source

### Build Server (Linux)

```bash
cd ExecTrace/backend

# Compile
g++ -std=c++17 -DCROW_USE_BOOST \
    -I include/crow_include \
    src/server.cpp \
    -o et-server \
    -pthread -lboost_system

# Run
./et-server
```

### Build SDK Test

```bash
cd ExecTrace/sdk

# Linux
g++ -std=c++17 test_sdk.cpp -o test_sdk -pthread
./test_sdk
```

## 🐳 Docker Deployment

### Quick Deploy

```bash
docker-compose up -d
```

### Database Reset
To clear all data and start fresh:
```bash
docker-compose down
rm backend/data/*.db
docker-compose up -d
```

## 🤝 Contributing

Contributions are welcome! Please open a Pull Request.

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Open a Pull Request

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

**Made with ❤️ by developers, for developers**
