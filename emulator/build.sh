#!/usr/bin/env bash
# Build script for the LoongArch emulator

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/.build"

# Parse arguments
BUILD_TYPE="Release"
NATIVE=""
LTO="-DLTO=ON"
MASKED_MEMORY_BITS=""
LA_DEBUG=""
LA_BINARY_TRANSLATION=""
LA_THREADED="-DLA_THREADED=ON"
LA_ARENA_BASE_REGISTER="-DLA_ENABLE_ARENA_BASE_REGISTER=OFF"
LA_TAILCALL="-DLA_TAILCALL_DISPATCH=OFF"

while [[ $# -gt 0 ]]; do
	case $1 in
		-d|--debug)
			BUILD_TYPE="Debug"
			LA_DEBUG="-DLA_DEBUG=ON"
			shift
			;;
		-n|--native)
			NATIVE="-DNATIVE=ON"
			shift
			;;
		--lto)
			LTO="-DLTO=ON"
			shift
			;;
		--no-lto)
			LTO="-DLTO=OFF"
			shift
			;;
		-N|--masked-memory-bits)
			MASKED_MEMORY_BITS="-DLA_MASKED_MEMORY_BITS=$2"
			shift 2
			;;
		--arena-base-register)
			LA_ARENA_BASE_REGISTER="-DLA_ENABLE_ARENA_BASE_REGISTER=ON"
			shift
			;;
		--bintr)
			LA_BINARY_TRANSLATION="-DLA_BINARY_TRANSLATION=ON"
			shift
			;;
		--binary-translation)
			LA_BINARY_TRANSLATION="-DLA_BINARY_TRANSLATION=ON"
			shift
			;;
		--no-bintr)
			LA_BINARY_TRANSLATION="-DLA_BINARY_TRANSLATION=OFF"
			shift
			;;
		--no-threaded)
			LA_THREADED="-DLA_THREADED=OFF"
			shift
			;;
		--tailcall-dispatch)
			LA_TAILCALL="-DLA_TAILCALL_DISPATCH=ON"
			shift
			;;
		-h|--help)
			echo "Usage: $0 [options]"
			echo ""
			echo "Build Options:"
			echo "  -d, --debug               Build in Debug mode (default: Release)"
			echo "  -n, --native              Enable native CPU optimizations (-march=native)"
			echo "  --no-lto                  Disable link-time optimization"
			echo ""
			echo "Library Options:"
			echo "  --masked-memory-bits N    Set masked memory arena size to 2^N bytes (0=disabled)"
			echo "                            Example: --masked-memory-bits 32 (4GB arena)"
			echo "  --binary-translation      Enable binary translation (experimental)"
			echo "  --no-threaded             Disable threaded dispatch"
			echo ""
			echo "Examples:"
			echo "  $0                                    # Standard optimized build"
			echo "  $0 -n                                 # With native optimizations"
			echo "  $0 --masked-memory-bits 32            # 4GB masked memory arena"
			echo "  $0 -n --masked-memory-bits 30         # 1GB arena with native opts"
			echo "  $0 -d                                 # Debug build"
			exit 0
			;;
		*)
			echo "Unknown option: $1"
			echo "Use --help for usage information"
			exit 1
			;;
	esac
done

echo "Building LoongArch emulator..."
echo "  Build type: $BUILD_TYPE"
[ -n "$NATIVE" ] && echo "  Native optimization: ON" || echo "  Native optimization: OFF"
echo "  LTO: ${LTO#-DLTO=}"
[ -n "$LA_DEBUG" ] && echo "  Debug mode: ON" || echo "  Debug mode: OFF"
[ -n "$LA_BINARY_TRANSLATION" ] && echo "  Binary translation: ON" || echo "  Binary translation: OFF"
echo "  Threaded dispatch: ${LA_THREADED#-DLA_THREADED=}"
if [ -n "$MASKED_MEMORY_BITS" ]; then
	BITS="${MASKED_MEMORY_BITS#-DLA_MASKED_MEMORY_BITS=}"
	SIZE=$((1 << BITS))
	if [ $SIZE -ge 1073741824 ]; then
		echo "  Masked memory: ${BITS} bits ($((SIZE / 1073741824))GB arena)"
	elif [ $SIZE -ge 1048576 ]; then
		echo "  Masked memory: ${BITS} bits ($((SIZE / 1048576))MB arena)"
	else
		echo "  Masked memory: ${BITS} bits (${SIZE} bytes)"
	fi
else
	echo "  Masked memory: Disabled (full range)"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
cmake .. \
	-DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
	$NATIVE \
	$LTO \
	$LA_DEBUG \
	$LA_BINARY_TRANSLATION \
	$LA_THREADED \
	$MASKED_MEMORY_BITS \
	$LA_ARENA_BASE_REGISTER \
	$LA_TAILCALL

# Build
make -j$(nproc)

echo ""
echo "Build complete! Binary: $BUILD_DIR/laemu"
echo ""
echo "Run with: ./laemu <program.elf>"
