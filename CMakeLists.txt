cmake_minimum_required(VERSION 3.10)
project(ExecTrace)

set(CMAKE_CXX_STANDARD 17)

include_directories(src)

add_executable(et-server src/server.cpp src/btree/DiskManager.cpp)

add_executable(et-cli src/cli.cpp)