#!/bin/bash

echo "Building ExecTrace SDK test..."

g++ -std=c++17 test_sdk.cpp -o test_sdk -pthread

if [ $? -eq 0 ]; then
    echo "✓ Build successful!"
    echo ""
    echo "Run with: ./test_sdk"
else
    echo "✗ Build failed"
    exit 1
fi
