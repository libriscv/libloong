#!/bin/bash
set -e

# Build script for libloong Rust bindings

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "======================================"
echo "Building libloong Rust bindings"
echo "======================================"
echo ""

# Step 1: Build libloong C++ library
echo "Step 1: Building libloong C++ library..."
cd "$PROJECT_ROOT"
mkdir -p build
cd build

if [ ! -f "Makefile" ] && [ ! -f "build.ninja" ]; then
    echo "Configuring CMake..."
    cmake .. -DCMAKE_BUILD_TYPE=Release -DLA_BINARY_TRANSLATION=ON
fi

echo "Building libloong..."
cmake --build . --target loong -j$(nproc)

echo "✓ libloong built successfully"
echo ""

# Step 2: Build Rust bindings
echo "Step 2: Building Rust bindings..."
cd "$SCRIPT_DIR"

cargo build --release

echo "✓ Rust bindings built successfully"
echo ""

# Step 3: Run tests (optional)
if [ "$1" == "--test" ] || [ "$1" == "-t" ]; then
    echo "Step 3: Running tests..."
    cargo test --release
    echo "✓ Tests passed"
    echo ""
fi

# Summary
echo "======================================"
echo "Build complete!"
echo "======================================"
echo ""
echo "Rust library: $SCRIPT_DIR/target/release/liblibloong.rlib"
echo "Examples:"
echo "  cargo run --example hello_world -- <elf_file>"
echo "  cargo run --example vmcall -- <elf_file> <function_name> [args...]"
echo ""
