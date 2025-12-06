#!/bin/bash
set -e

echo "Building Asteroid Dodge game (Rust guest)..."

# Set up target if not already added
rustup target add loongarch64-unknown-linux-gnu 2>/dev/null || true

if [ ! -f libloong_api.rs ]; then
	echo "Error: The generated API file 'libloong_api.rs' is missing."
	echo "Please generate the bindings first by running the main game with --generate-bindings."
	exit 1
fi

# Build the project
CARGO_PROFILE_RELEASE_DEBUG=true cargo build --release --target loongarch64-unknown-linux-gnu

# Copy the binary to a convenient location
cp target/loongarch64-unknown-linux-gnu/release/asteroid_dodge game.elf

echo "Build complete: game.elf"
echo "File size: $(stat -f%z game.elf 2>/dev/null || stat -c%s game.elf)"
