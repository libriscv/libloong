#!/bin/bash
set -e

# Simple build script for libloong benchmark suite

echo "Building libloong benchmark suite..."
echo

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Configure
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo "Building..."
make -j$(nproc)

echo
echo "Build complete!"
echo "Run ./build/bench to execute benchmarks"
echo "Run ./build/bench --help for options"
