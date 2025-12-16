#!/bin/sh
# Build script for the LoongArch emulator
# POSIX-compliant for portability (Linux, FreeBSD, etc.)

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/.build"

# Parse arguments
BUILD_TYPE="Release"
NATIVE=""
LTO="-DLTO=ON"
MASKED_MEMORY_BITS=""
LA_DEBUG=""
LA_THREADED="-DLA_THREADED=ON"
LA_TAILCALL="-DLA_TAILCALL_DISPATCH=OFF"
LA_LASX="-DLA_LASX=OFF"
LA_EMBED_BINTR="-DLA_EMBED_BINTR=\"\""

while [ $# -gt 0 ]; do
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
		--no-native)
			NATIVE="-DNATIVE=OFF"
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
		--threaded)
			LA_THREADED="-DLA_THREADED=ON"
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
		--lasx)
			LA_LASX="-DLA_LASX=ON"
			shift
			;;
		--no-lasx)
			LA_LASX="-DLA_LASX=OFF"
			shift
			;;
		--embed)
			# Convert relative path to absolute path
			if [ -n "$2" ]; then
				if [ -f "$2" ]; then
					LA_EMBED_BINTR="-DLA_EMBED_BINTR=$(cd "$(dirname "$2")" && pwd)/$(basename "$2")"
				else
					echo "Error: Embedded bintr file not found: $2"
					exit 1
				fi
			else
				echo "Error: --embed requires a file path"
				exit 1
			fi
			shift 2
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
			echo "  --threaded                Enable threaded dispatch"
			echo "  --no-threaded             Disable threaded dispatch"
			echo "  --tailcall-dispatch       Enable tailcall dispatch method"
			echo "  --lasx                    Enable LASX instruction support"
			echo "  --no-lasx                 Disable LASX instruction support"
			echo "  --embed <file.c>          Embed pre-compiled binary translation from file"
			echo ""
			echo "Examples:"
			echo "  $0                                    # Standard optimized build"
			echo "  $0 -n                                 # With native optimizations"
			echo "  $0 --masked-memory-bits 32            # 4GB masked memory arena"
			echo "  $0 -n --masked-memory-bits 30         # 1GB arena with native opts"
			echo "  $0 -d                                 # Debug build"
			echo "  $0 --embed program_bintr.c            # Build with embedded translation"
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
if [ -n "$NATIVE" ]; then
	echo "  Native optimization: ON"
else
	echo "  Native optimization: OFF"
fi
# Extract value after the = sign
LTO_VALUE=$(echo "$LTO" | sed 's/.*=//')
echo "  LTO: $LTO_VALUE"
if [ -n "$LA_DEBUG" ]; then
	echo "  Debug mode: ON"
else
	echo "  Debug mode: OFF"
fi
THREADED_VALUE=$(echo "$LA_THREADED" | sed 's/.*=//')
echo "  Threaded dispatch: $THREADED_VALUE"
if [ -n "$LA_EMBED_BINTR" ]; then
	EMBED_FILE=$(echo "$LA_EMBED_BINTR" | sed 's/.*=//')
	echo "  Embedded bintr: $EMBED_FILE"
fi
if [ -n "$MASKED_MEMORY_BITS" ]; then
	BITS=$(echo "$MASKED_MEMORY_BITS" | sed 's/.*=//')
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
	-DLA_BINARY_TRANSLATION=ON \
	-DLA_CPU_RELATIVE_MEMORY=ON \
	$LA_THREADED \
	$MASKED_MEMORY_BITS \
	$LA_TAILCALL \
	$LA_LASX \
	$LA_EMBED_BINTR

# Build
# Detect number of CPU cores in a portable way
if command -v nproc >/dev/null 2>&1; then
	NCPUS=$(nproc)
elif command -v sysctl >/dev/null 2>&1; then
	# FreeBSD, macOS
	NCPUS=$(sysctl -n hw.ncpu 2>/dev/null || echo 1)
else
	# Fallback
	NCPUS=1
fi
make -j${NCPUS}

echo ""
echo "Build complete! Binary: $BUILD_DIR/laemu"
echo ""
echo "Run with: ./laemu <program.elf>"
