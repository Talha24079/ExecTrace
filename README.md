# ExecTrace

ExecTrace is a lightweight, secure C++ backend for tracing and logging execution events. It provides a robust API for registering users, logging trace data (function calls, RAM usage, duration), and querying logs with powerful filtering capabilities.

Designed for performance and simplicity, ExecTrace supports B-Tree based indexing for efficient queries and includes a command-line interface (CLI) for easy interaction.

## Features

-   **High Performance**: C++ backend optimized for speed.
-   **Structured Logging**: Log function names, messages, RAM/ROM usage, and execution duration.
-   **Efficient Querying**: Retrieve logs by time range, RAM usage, or duration.
-   **Persistence**: Data survives restarts using SQLite-compatible local storage.
-   **Containerized**: Docker and Docker Compose support for easy deployment.
-   **CLI Tool**: Built-in CLI for quick testing and interaction.

## Getting Started

### Prerequisites

-   **Docker** (Recommended)
-   **C++ Compiler** (GCC/Clang, C++17 support) & **CMake** (For local build)

### Installation

#### Option 1: Docker Compose (Recommended)

1.  Clone the repository:
    ```bash
    git clone https://github.com/yourusername/ExecTrace.git
    cd ExecTrace
    ```
2.  Start the service:
    ```bash
    docker compose up -d
    ```
    The server will be available at `http://localhost:8080`. Data is persisted in the `./data` directory.

#### Option 2: Docker Build

1.  Build the image:
    ```bash
    docker build -t exectrace-backend .
    ```
2.  Run the container with persistence:
    ```bash
    mkdir -p data
    docker run -d -p 8080:8080 -v $(pwd)/data:/data exectrace-backend
    ```

#### Option 3: Local Build

1.  Run the build script:
    ```bash
    ./build.sh
    ```
    This compiles the server and CLI, then starts the server in the background.

## Usage

You can interact with the server using the built-in CLI tool found in `build/et-cli`.

### 1. Register a User
Get an API Key to authenticate your requests.

```bash
./build/et-cli register <username>
# Example: ./build/et-cli register dev_user
```

### 2. Log Trace Data
Log an execution event.

```bash
./build/et-cli log <api_key> <func_name> <ram_mb> <duration_ms>
# Example: ./build/et-cli log fa37... main 128 500
```

### 3. Query Logs
Retrieve logs based on criteria.

**By RAM Usage:**
```bash
./build/et-cli query <api_key> ram <min_mb> <max_mb>
# Example: ./build/et-cli query fa37... ram 0 1000
```

**By Duration:**
```bash
./build/et-cli query <api_key> duration <min_ms> <max_ms>
# Example: ./build/et-cli query fa37... duration 0 2000
```

## Support

For issues and feature requests, please check the [Issues](https://github.com/yourusername/ExecTrace/issues) page or consult the source code in `src/`.

## Contributing

We welcome contributions! Please read our [CONTRIBUTING.md](docs/CONTRIBUTING.md) (if available) for details on our code of conduct and the process for submitting pull requests.

## Maintainers

-   **Talha** - *Initial Work*
