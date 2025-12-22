# Backend Analysis Report: ExecTrace

## Overview
**ExecTrace** is a high-performance C++ backend designed for logging and querying execution traces. It uses a custom B-Tree database for storage and `httplib` for the HTTP server.

-   **Server Host/Port**: `0.0.0.0:8080` (Docker container usually maps this to localhost:8080)
-   **Base URL**: `http://localhost:8080`
-   **Database Location**: `./data/` (or specified via command line)

## Authentication
Most API endpoints require authentication via an API Key.
-   **Header**: `X-API-Key: <your_api_key>`
-   **Validation**: The server checks this key against `sys_users.db`. Valid keys map to a specific `user_id`.

## API Endpoints

### 1. System Status
Checks if the server is running.
-   **Method**: `GET`
-   **Path**: `/status`
-   **Auth Required**: No
-   **Response**:
    ```json
    {
        "status": "running", 
        "auth": "enabled"
    }
    ```

### 2. User Registration
Registers a new user and generates an API Key.
-   **Method**: `POST`
-   **Path**: `/register`
-   **Auth Required**: No
-   **Parameters** (Form Data / URL Encoded):
    -   `username` (string): The desired username.
-   **Response**:
    ```json
    {
        "username": "dev_user", 
        "api_key": "aiRrV4..."
    }
    ```

### 3. Log a Trace
Saves a new execution trace entry.
-   **Method**: `POST`
-   **Path**: `/log`
-   **Auth Required**: Yes
-   **Parameters** (Form Data):
    -   `func` (string): Function name.
    -   `msg` (string): Log message.
    -   `duration` (int): Duration in milliseconds.
    -   `ram` (int): RAM usage in KB.
    -   `rom` (int): ROM usage (optional, usually 0).
-   **Response**:
    ```json
    {
        "result": "saved", 
        "id": 123456
    }
    ```

### 4. Query Logs
Retrieves logs based on various criteria. All return a JSON array of trace objects.

| Endpoint | Logs Returned | Parameters |
| :--- | :--- | :--- |
| `/query` | By ID Range | `start` (int, default 0), `end` (int, default MAX) |
| `/query/ram` | By RAM Usage | `min` (int), `max` (int) |
| `/query/duration`| By Duration | `min` (int), `max` (int) |
| `/query/heavy` | Intersection | `min_ram` (int), `min_dur` (int) |

**Response Format (JSON Array):**
```json
[
    {
        "id": 1001,
        "user_id": 1,
        "version": 1,
        "func": "main",
        "msg": "Initialization",
        "ram": 1024,
        "rom": 0,
        "duration": 500
    },
    ...
]
```

## Static File Serving
The C++ server also acts as a static file server for the simple frontend.

| URL Path | File Served | Description |
| :--- | :--- | :--- |
| `/` | `frontend/index.html` | Landing Page |
| `/docs` | `frontend/docs.html` | Documentation |
| `/dashboard` | `frontend/dashboard.html` | Live Dashboard |
| `/download/sdk`| `frontend/libexectrace.zip` | SDK Download |

> **Note**: These files must exist in the `frontend/` directory relative to the server executable's working directory.

## Persistence
-   Data is stored in `data/`.
-   Files:
    -   `sys_users.db`: User accounts and API keys.
    -   `sys_logs`: Main log storage.
    -   `sys_logs_ram.db`: Index for RAM queries.
    -   `sys_logs_dur.db`: Index for duration queries.
-   Docker volume mounting (`-v $(pwd)/data:/data`) ensures these files persist across container restarts.
