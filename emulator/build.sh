#!/usr/bin/env bash
# Build script for the LoongArch emulator

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/.build"

# Parse arguments
BUILD_TYPE="Release"
NATIVE=""
LTO="-DLTO=ON"

while [[ $# -gt 0 ]]; do
	case $1 in
		-d|--debug)
			BUILD_TYPE="Debug"
			shift
			;;
		-n|--native)
			NATIVE="-DNATIVE=ON"
			shift
			;;
		--no-lto)
			LTO="-DLTO=OFF"
			shift
			;;
		*)
			echo "Unknown option: $1"
			echo "Usage: $0 [-d|--debug] [-n|--native] [--no-lto]"
			exit 1
			;;
	esac
done

echo "Building LoongArch emulator..."
echo "  Build type: $BUILD_TYPE"
echo "  Native optimization: ${NATIVE:-OFF}"
echo "  LTO: ${LTO}"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
cmake .. \
	-DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
	$NATIVE \
	$LTO

# Build
make -j$(nproc)

echo ""
echo "Build complete! Binary: $BUILD_DIR/laemu"
echo ""
echo "Run with: ./laemu <program.elf>"
