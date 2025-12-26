# ExecTrace

> A lightweight, cross-platform C++ profiler for real-time function tracing and performance monitoring.

[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-blue.svg)](https://github.com)
[![Language](https://img.shields.io/badge/language-C%2B%2B17-orange.svg)](https://github.com)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
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

```
ExecTrace/
├── sdk/                    # C++ SDK (header-only)
│   ├── exectrace.hpp      # Main SDK header
│   └── test_sdk.cpp       # SDK usage examples
├── backend/               # C++ Server (Crow framework)
│   ├── src/
│   │   └── server.cpp     # Main server implementation
│   ├── include/
│   │   ├── Database.hpp   # Trace database interface
│   │   ├── AuthDB.hpp     # User & project management
│   │   └── crow_include/  # Crow web framework
│   └── data/              # SQLite databases (auto-created)
├── frontend/              # Web Dashboard
│   ├── login.html         # Authentication page
│   ├── dashboard.html     # Main analytics view
│   └── workspace.html     # Project management
└── docker-compose.yml     # Container orchestration
```

### Communication Flow

```
[Your C++ App] ──(1)──> [ExecTrace SDK] ──(HTTP/curl)──> [Server:8080] ──> [SQLite DB]
                                                                ↓
                                                         [Dashboard UI]
```

1. **Client Side**: Your app uses SDK macros/functions to trace execution
2. **Transport**: SDK sends trace data via HTTP POST with API key
3. **Server Side**: Crow-based server receives, parses, and stores traces
4. **Visualization**: Web dashboard queries server for analytics

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

Navigate to `http://localhost:9090/dashboard` to see your traces in real-time!

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

### Platform-Specific Setup

#### Windows

1. **Install Visual Studio 2019+** with C++ build tools
2. **Install vcpkg** (for Boost):
   ```cmd
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg integrate install
   .\vcpkg install boost:x64-windows
   ```

3. **Copy SDK header** to your project:
   ```cmd
   copy ExecTrace\sdk\exectrace.hpp YourProject\include\
   ```

#### Linux

1. **Install dependencies**:
   ```bash
   sudo apt update
   sudo apt install build-essential libboost-all-dev
   ```

2. **Copy SDK header**:
   ```bash
   cp ExecTrace/sdk/exectrace.hpp your_project/include/
   ```

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

### Server Configuration

The server runs on port **8080** by default (mapped to **9090** in Docker). Key endpoints:

- `GET /` - Dashboard landing page
- `POST /log` - Submit trace data
- `GET /logs/<project_id>` - Retrieve project traces
- `GET /health` - Health check

### API Key Management

1. Navigate to `http://localhost:9090/login`
2. Create an account or login
3. Create a new project to get an API key
4. Use the API key in your SDK initialization:

```cpp
ExecTrace::init("abc123xyz456", "v1.0.0");
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

#### Utility Functions

```cpp
long ExecTrace::get_current_ram_kb();  // Get current process RAM usage
std::string ExecTrace::auto_version(); // Auto-detect version from ENV
```

### REST API Endpoints

#### POST /log
Submit a trace event.

**Headers**:
- `X-API-Key`: Your project API key

**Body** (URL-encoded):
```
func=functionName&message=description&duration=123&ram=4096&version=v1.0.0
```

**Response**:
```json
{
  "status": "ok",
  "id": 42
}
```

#### GET /logs/:project_id
Retrieve traces for a project.

**Response**:
```json
{
  "status": "ok",
  "count": 100,
  "logs": [
    {
      "id": 1,
      "project_id": 1,
      "func": "processData",
      "message": "Auto-traced",
      "app_version": "v1.0.0",
      "duration": 125,
      "ram_usage": 8192,
      "timestamp": 1703597182
    }
  ]
}
```

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

### Build Server (Windows)

```cmd
cd ExecTrace\backend

cl /std:c++17 /EHsc /DCROW_USE_BOOST ^
   /I include\crow_include ^
   src\server.cpp ^
   /Fe:et-server.exe ^
   /link /LIBPATH:C:\vcpkg\installed\x64-windows\lib

et-server.exe
```

### Build SDK Test

```bash
cd ExecTrace/sdk

# Linux
g++ -std=c++17 test_sdk.cpp -o test_sdk -pthread
./test_sdk

# Windows
cl /std:c++17 test_sdk.cpp /Fe:test_sdk.exe
test_sdk.exe
```

## 🐳 Docker Deployment

### Quick Deploy

```bash
docker-compose up -d
```

### Custom Configuration

Edit `docker-compose.yml` to customize ports:

```yaml
services:
  exectrace:
    build: .
    ports:
      - "YOUR_PORT:8080"  # Change 9090 to your preferred port
    restart: unless-stopped
```

### View Logs

```bash
docker logs exectrace-server
```

### Rebuild

```bash
docker-compose up -d --build
```

## 🔧 Platform-Specific Notes

### Windows Compatibility

ExecTrace v3.1+ includes full Windows support:

- ✅ SDK uses proper cmd.exe syntax (double quotes)
- ✅ Platform-specific directory handling
- ✅ Visual Studio project integration
- ✅ Working directory detection on startup

### Linux Optimizations

- Uses `/dev/null` for silent curl execution
- POSIX-compliant directory operations
- Systemd service support (optional)

## 🐛 Troubleshooting

### "Database files not found"

The server prints the current working directory on startup. Check the console output:

```
[Server] Current Working Directory: /path/to/your/exe
```

Database files are in `backend/data/` relative to this path.

### "API key not working"

**Current Status**: API keys are logged but project ID mapping is not yet implemented. All traces default to Project ID 1 with a warning message. This is a known limitation and will be addressed in a future update.

### "Curl command fails on Windows"

Ensure you're using the latest SDK (v3.1+) which includes Windows-specific curl syntax.

## 🤝 Contributing

Contributions are welcome! Please follow these steps:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

### Development Priorities

- [ ] Implement full API key → Project ID mapping
- [ ] Add gRPC transport option (faster than HTTP)
- [ ] Create SDKs for other languages (Python, Go, Rust)
- [ ] Real-time WebSocket streaming
- [ ] Advanced analytics and ML-based anomaly detection

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- [Crow](https://github.com/CrowCpp/Crow) - Fast and easy to use C++ micro web framework
- [SQLite](https://www.sqlite.org/) - Embedded database engine
- [Boost](https://www.boost.org/) - C++ libraries

## 📧 Contact

Project Link: [https://github.com/yourusername/ExecTrace](https://github.com/yourusername/ExecTrace)

---

**Made with ❤️ by developers, for developers**
