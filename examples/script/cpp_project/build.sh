#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure CMake
echo "Configuring CMake..."
cmake "$SCRIPT_DIR" -DCMAKE_BUILD_TYPE=Release

# Build
echo "Building..."
cmake --build . -j$(nproc)

# Copy output
cp guest_app "$SCRIPT_DIR/guest_app.elf"

echo ""
echo "Build complete! Executable: guest_app.elf"
echo ""
