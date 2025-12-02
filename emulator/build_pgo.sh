#!/usr/bin/env bash
# PGO build script for the LoongArch emulator using CoreMark profiling
# This script performs a 3-stage build:
# 1. Build with profiling instrumentation
# 2. Run CoreMark to collect profile data
# 3. Rebuild using profile data for optimizations

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/.build_pgo"
PROFILE_DIR="${BUILD_DIR}/pgo_profiles"
COREMARK_ELF="${SCRIPT_DIR}/../tests/programs/coremark.elf"

# Parse arguments - same as build.sh
BUILD_TYPE="Release"
NATIVE=""
LTO="-DLTO=ON"
MASKED_MEMORY_BITS="-DLA_MASKED_MEMORY_BITS=0"
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
		--binary-translation)
			LA_BINARY_TRANSLATION="-DLA_BINARY_TRANSLATION=ON"
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
			echo "PGO Build Script - Profiles with CoreMark for optimal performance"
			echo ""
			echo "Build Options:"
			echo "  -d, --debug               Build in Debug mode (default: Release)"
			echo "  -n, --native              Enable native CPU optimizations (-march=native)"
			echo "  --no-lto                  Disable link-time optimization"
			echo ""
			echo "Library Options:"
			echo "  -N, --masked-memory-bits N    Set masked memory arena size to 2^N bytes (0=disabled)"
			echo "                                Example: -N 32 (4GB arena)"
			echo "  --binary-translation          Enable binary translation (experimental)"
			echo "  --no-threaded                 Disable threaded dispatch"
			echo "  --arena-base-register         Enable arena base register optimization"
			echo "  --tailcall-dispatch           Enable tailcall dispatch"
			echo ""
			echo "Examples:"
			echo "  $0                            # Standard PGO build"
			echo "  $0 -n                         # With native optimizations"
			echo "  $0 --no-lto -N 0              # Match current best config"
			exit 0
			;;
		*)
			echo "Unknown option: $1"
			echo "Use --help for usage information"
			exit 1
			;;
	esac
done

# Check if CoreMark exists
if [ ! -f "$COREMARK_ELF" ]; then
	echo "Error: CoreMark binary not found at $COREMARK_ELF"
	exit 1
fi

echo "========================================"
echo "PGO Build for LoongArch Emulator"
echo "========================================"
echo "  Build type: $BUILD_TYPE"
[ -n "$NATIVE" ] && echo "  Native optimization: ON" || echo "  Native optimization: OFF"
echo "  LTO: ${LTO#-DLTO=}"
[ -n "$LA_DEBUG" ] && echo "  Debug mode: ON" || echo "  Debug mode: OFF"
[ -n "$LA_BINARY_TRANSLATION" ] && echo "  Binary translation: ON" || echo "  Binary translation: OFF"
echo "  Threaded dispatch: ${LA_THREADED#-DLA_THREADED=}"
if [ -n "$MASKED_MEMORY_BITS" ]; then
	BITS="${MASKED_MEMORY_BITS#-DLA_MASKED_MEMORY_BITS=}"
	if [ "$BITS" != "0" ]; then
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
fi
echo "========================================"

# Clean up old profile data
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
mkdir -p "$PROFILE_DIR"

# Detect compiler type for PGO flags
COMPILER_TYPE="unknown"
if command -v "$CXX" &> /dev/null; then
	if "$CXX" --version 2>&1 | grep -qi "clang"; then
		COMPILER_TYPE="clang"
	elif "$CXX" --version 2>&1 | grep -qi "gcc"; then
		COMPILER_TYPE="gcc"
	fi
elif command -v clang++ &> /dev/null && clang++ --version 2>&1 | grep -qi "clang"; then
	COMPILER_TYPE="clang"
elif command -v g++ &> /dev/null && g++ --version 2>&1 | grep -qi "gcc"; then
	COMPILER_TYPE="gcc"
fi

# Set PGO flags based on compiler
if [ "$COMPILER_TYPE" = "clang" ]; then
	echo "Using Clang PGO (LLVM instrumentation)"
	PROFILE_GENERATE_FLAGS="-fprofile-instr-generate"
	PROFILE_USE_FLAGS="-fprofile-instr-use=${PROFILE_DIR}/merged.profdata"
	PROFILE_RAW_FILE="${PROFILE_DIR}/default.profraw"
	PROFILE_DATA_FILE="${PROFILE_DIR}/merged.profdata"
else
	echo "Using GCC PGO"
	PROFILE_GENERATE_FLAGS="-fprofile-generate=${PROFILE_DIR}"
	PROFILE_USE_FLAGS="-fprofile-use=${PROFILE_DIR} -fprofile-correction"
	PROFILE_RAW_FILE=""
	PROFILE_DATA_FILE="${PROFILE_DIR}/*.gcda"
fi

# Stage 1: Build with profiling instrumentation
echo ""
echo "Stage 1/3: Building with profiling instrumentation..."
echo "========================================"
cd "$BUILD_DIR"

cmake .. \
	-DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
	-DCMAKE_CXX_FLAGS="$PROFILE_GENERATE_FLAGS" \
	-DCMAKE_EXE_LINKER_FLAGS="$PROFILE_GENERATE_FLAGS" \
	$NATIVE \
	-DLTO=OFF \
	$LA_DEBUG \
	$LA_BINARY_TRANSLATION \
	$LA_THREADED \
	$MASKED_MEMORY_BITS \
	$LA_ARENA_BASE_REGISTER \
	$LA_TAILCALL

make -j$(nproc)

# Stage 2: Run CoreMark to collect profile data
echo ""
echo "Stage 2/3: Running CoreMark to collect profile data..."
echo "========================================"
echo "This may take a minute..."

# Set LLVM_PROFILE_FILE for Clang
if [ "$COMPILER_TYPE" = "clang" ]; then
	export LLVM_PROFILE_FILE="$PROFILE_RAW_FILE"
fi

./laemu "$COREMARK_ELF" > /dev/null 2>&1 || true

# Process profile data based on compiler type
if [ "$COMPILER_TYPE" = "clang" ]; then
	# Merge Clang profile data
	if [ ! -f "$PROFILE_RAW_FILE" ]; then
		echo "Error: No profile data generated at $PROFILE_RAW_FILE"
		exit 1
	fi
	echo "Merging profile data..."
	llvm-profdata-20 merge -output="$PROFILE_DATA_FILE" "$PROFILE_RAW_FILE"
	if [ ! -f "$PROFILE_DATA_FILE" ]; then
		echo "Error: Failed to merge profile data!"
		exit 1
	fi
	echo "Profile data merged: $PROFILE_DATA_FILE"
else
	# Check GCC profile data
	PROFILE_COUNT=$(find "$PROFILE_DIR" -name "*.gcda" 2>/dev/null | wc -l)
	if [ "$PROFILE_COUNT" -eq 0 ]; then
		echo "Error: No profile data generated!"
		echo "Profile directory: $PROFILE_DIR"
		exit 1
	fi
	echo "Profile data collected: $PROFILE_COUNT files"
fi

# Stage 3: Rebuild with profile-guided optimizations
echo ""
echo "Stage 3/3: Rebuilding with profile-guided optimizations..."
echo "========================================"

# Clean build artifacts but keep profile data
make clean

cmake .. \
	-DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
	-DCMAKE_CXX_FLAGS="$PROFILE_USE_FLAGS" \
	-DCMAKE_EXE_LINKER_FLAGS="$PROFILE_USE_FLAGS" \
	$NATIVE \
	$LTO \
	$LA_DEBUG \
	$LA_BINARY_TRANSLATION \
	$LA_THREADED \
	$MASKED_MEMORY_BITS \
	$LA_ARENA_BASE_REGISTER \
	$LA_TAILCALL

make -j$(nproc)

echo ""
echo "========================================"
echo "PGO Build Complete!"
echo "========================================"
echo "Binary: $BUILD_DIR/laemu"
echo ""
echo "Testing with CoreMark..."
echo "========================================"
echo ""
"$BUILD_DIR/laemu" "$COREMARK_ELF"
echo ""
echo "========================================"
echo "To compare with non-PGO build, run:"
echo "  ./build.sh ${NATIVE} ${LTO} ${MASKED_MEMORY_BITS} ${LA_THREADED} && .build/laemu ../tests/programs/coremark.elf"
echo ""
