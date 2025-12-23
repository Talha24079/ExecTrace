# ExecTrace - Real-Time C++ Profiler

Clean, minimalist SaaS platform for profiling C++ applications in real-time.

## Project Structure

```
/ExecTrace
├── /frontend          # Web UI
│   ├── index.html     # Landing page
│   ├── register.html  # Signup
│   ├── login.html     # Login
│   ├── workspace.html # Projects
│   └── dashboard.html # Analytics
├── /backend           # C++ Server
│   ├── /include       # Headers
│   ├── /src           # Server code
│   ├── /data          # Database files
│   └── build.sh       # Compile script
└── /sdk_new           # Client SDK
    ├── exectrace.hpp
    ├── exectrace.cpp
    └── test_seeder.cpp
```

## Quick Start

### 1. Run Server

```bash
cd backend
bash build.sh
./et-server
```

Server runs on `http://localhost:8080`

### 2. Create Account

Visit `http://localhost:8080/register` and create an account with email/password.

### 3. Create Project

Login and create a new project. Copy the Project API Key.

### 4. Use SDK

```cpp
#include "exectrace.hpp"

int main() {
    ExecTrace::init("your_project_key", "v1.0.0");
    TRACE_FUNCTION();
    return 0;
}
```

### 5. View Dashboard

Open `http://localhost:8080/dashboard?key=your_project_key`

## Docker

```bash
docker-compose up --build
```

## Authentication

- **Email/Password**: For user accounts (register/login)
- **Project Keys**: For SDK integration  

## Features

- B-Tree database (no external dependencies)
- Email/password authentication with hashing
- Real-time dashboard
- Persistent HTTP connections
- Cross-platform SDK

## Tech Stack

- Backend: C++17, httplib
- Frontend: HTML/CSS/JavaScript (Light Mode)
- Database: Custom B-Tree implementation
