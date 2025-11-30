#!/bin/bash
set -e

echo "Building Rust LoongArch example..."

# Build the std binary for LoongArch
cargo build --release --bin hello-std

# Copy the binary to a convenient location
cp target/loongarch64-unknown-linux-gnu/release/hello-std ./hello-std.elf

echo "Build complete!"
echo ""
echo "Built binary:"
echo "  hello-std.elf (using std with libc)"
echo ""
echo "To run with libloong:"
echo "  ../../emulator/.build/laemu hello-std.elf"
echo ""
echo "To debug with debug_test:"
echo "  ../../build/tests/debug_test hello-std.elf"
