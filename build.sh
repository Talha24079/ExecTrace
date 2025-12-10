#!/bin/bash

mkdir -p build
cd build

echo "--- Configuring with CMake ---"
cmake ..

echo "--- Building project ---"
make

if [ $? -ne 0 ]; then
    echo "Error: Build failed."
    exit 1
fi

echo "--- Starting Server ---"
./et-server &
SERVER_PID=$!
