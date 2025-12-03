#!/bin/bash
set -e
echo "Building Rust LoongArch guest application..."

# Build the binary for LoongArch
# Note: Flags are configured in .cargo/config.toml (auto-generated)
cargo build --bin guest_app --release

# Copy the binary to a convenient location
cp target/loongarch64-unknown-linux-gnu/release/guest_app ./guest_app.elf

echo "Build complete!"
echo ""
echo "Built binary: guest_app.elf"
echo ""
