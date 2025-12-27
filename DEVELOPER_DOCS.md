# 🛠️ ExecTrace Developer Documentation

## 📋 Overview
ExecTrace is an execution tracing and performance monitoring system. It provides real-time insights into function execution times, RAM usage, and application performance.

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

## 🚀 Getting Started

### Prerequisites
- Docker & Docker Compose
- C++17 Compiler (for local dev)

### Running with Docker (Recommended)
```bash
# Build and start
docker-compose up -d --build

# View logs
docker-compose logs -f

# Stop
docker-compose down
```

The server runs on port **9090**.

### Database Reset
To clear all data and start fresh:
```bash
docker-compose down
rm backend/data/*.db
docker-compose up -d
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

## 🔌 API Summary

### Auth
- `POST /api/auth/register` - Create account
- `POST /api/auth/login` - Login

### Projects
- `POST /api/projects` - Create project
- `GET /api/projects` - List user projects
- `POST /api/projects/:id/settings` - Update thresholds

### Admin
- `GET /api/admin/users` - List all users
- `POST /api/admin/users/:id/role` - Update user role
- `POST /api/admin/users/:id/deactivate` - Deactivate user

### Tracing (SDK)
- `POST /api/trace` - Ingest performance data

## 🧪 Testing
Run the test suite to verify core functionality:
```bash
g++ -std=c++17 test.cpp -o test_suite
./test_suite
```
