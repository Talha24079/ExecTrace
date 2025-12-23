#!/bin/bash

cd "$(dirname "$0")"

echo "Building ExecTrace Backend..."

g++ -std=c++17 src/server.cpp -o et-server -pthread

if [ $? -eq 0 ]; then
    echo "✅ Build successful"
    echo "Run: ./et-server"
else
    echo "❌ Build failed"
    exit 1
fi
