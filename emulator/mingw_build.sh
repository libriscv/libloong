#!/bin/sh
# MinGW cross-compilation build script
# POSIX-compliant for portability (Linux, FreeBSD, etc.)

set -e
export CC=x86_64-w64-mingw32-gcc
export CXX=x86_64-w64-mingw32-g++

mkdir -p .build_mingw
cd .build_mingw
cmake .. -DCMAKE_BUILD_TYPE=Release -DLA_BINARY_TRANSLATION=ON -DCMAKE_TOOLCHAIN_FILE=../mingw_toolchain.cmake

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
cd ..

