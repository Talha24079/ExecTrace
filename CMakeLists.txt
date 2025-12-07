cmake_minimum_required(VERSION 3.10)

project(ExecTrace VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

find_package(Threads REQUIRED)

add_executable(ExecTraceCLI 
    src/main.cpp 
    src/btree/BTree.cpp 
    src/btree/DiskManager.cpp
)
target_link_libraries(ExecTraceCLI PRIVATE Threads::Threads)

add_executable(ExecTraceServer 
    src/server.cpp 
    src/btree/BTree.cpp 
    src/btree/DiskManager.cpp
)
target_link_libraries(ExecTraceServer PRIVATE Threads::Threads)