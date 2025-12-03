#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/.build"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure and build
echo "Configuring CMake..."
cmake "$SCRIPT_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=20

echo "Building..."
cmake --build . -j$(nproc)

ln -fs "$BUILD_DIR"/script_example "$SCRIPT_DIR/script_example"

echo ""
echo "Build complete! Executable: ./script_example"
echo ""
