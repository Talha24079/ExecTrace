#!/bin/bash

mkdir -p build
cd build


echo "--- Stopping any existing server ---"
pkill -f et-server || true

echo "--- Configuring with CMake ---"
cmake ..

echo "--- Building project ---"
make

if [ $? -ne 0 ]; then
    echo "Error: Build failed."
    exit 1
fi

echo "--- Starting Server ---"
# Pass "../" so files are created in project root, not build/
./et-server ../ &
SERVER_PID=$!
