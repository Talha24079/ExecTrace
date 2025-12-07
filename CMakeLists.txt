cmake_minimum_required(VERSION 3.10)

# Project Name and Version
project(ExecTrace VERSION 0.1 LANGUAGES CXX)

# Enforce C++ 20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add our header directory to the include path
include_directories(include)

# Define the source files
set(SOURCES
    src/main.cpp
)

# Create the executable
add_executable(ExecTrace ${SOURCES})
