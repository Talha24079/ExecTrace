#!/bin/bash

# 1. Stop existing server
echo "Stopping old server..."
pkill -f et-server

# 2. Setup Directories
mkdir -p build
mkdir -p frontend
mkdir -p data

# 3. Clean Build
echo "Building project..."
cd build
cmake ..
make
cd ..

# 4. Package SDK for Download
echo "Packaging SDK..."
if command -v zip >/dev/null 2>&1; then
    rm -f frontend/libexectrace.zip
    zip -r frontend/libexectrace.zip sdk/include sdk/src
    echo "✅ SDK Zipped."
else
    echo "⚠️ 'zip' command not found. Install zip to enable SDK downloads."
fi

# 5. Check if frontend files exist
if [ ! -f frontend/index.html ]; then
    echo "⚠️ Warning: Frontend files missing in 'frontend/' folder."
fi

echo "Build Complete. Run: ./build/et-server"