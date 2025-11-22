# Building libloong

## Prerequisites

### Linux / macOS

- CMake 3.5 or later
- C++20 compatible compiler:
  - GCC 10+ or
  - Clang 12+ or
  - Apple Clang 13+
- Make or Ninja build system

Install on Ubuntu/Debian:
```bash
sudo apt install cmake g++ make
```

Install on macOS:
```bash
brew install cmake
```

### Windows

- Visual Studio 2019 or later with C++ support
- CMake 3.5 or later
- Or use WSL with Linux instructions

## Quick Start

```bash
# Clone the repository
git clone https://github.com/your-org/libloong.git
cd libloong

# Build the library
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Optionally install system-wide
sudo make install
```

## Build Options

Configure with CMake options:

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DLA_32=ON \
  -DLA_64=ON \
  -DLA_DEBUG=OFF \
  -DLA_BINARY_TRANSLATION=OFF \
  -DLA_THREADED=ON \
  -DLA_MEMORY_TRAPS=ON
```

### Available Options

| Option | Default | Description |
|--------|---------|-------------|
| `LA_32` | ON | Enable LA32 (32-bit) support |
| `LA_64` | ON | Enable LA64 (64-bit) support |
| `LA_DEBUG` | OFF | Enable debug output and logging |
| `LA_BINARY_TRANSLATION` | OFF | Enable binary translation (faster) |
| `LA_THREADED` | ON | Enable threading support |
| `LA_MEMORY_TRAPS` | ON | Enable memory access traps |

## Build Types

### Debug Build
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```
- No optimizations
- Full debug symbols
- Assertions enabled

### Release Build
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```
- Full optimizations (-O3)
- No debug symbols
- Best performance

### RelWithDebInfo Build
```bash
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j$(nproc)
```
- Optimized with debug symbols
- Good for profiling

## Building the Emulator

The standalone emulator is built automatically:

```bash
cd emulator
./build.sh

# Or manually:
mkdir .build && cd .build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run the emulator
./laemu path/to/program.elf
```

### Emulator Build Options

```bash
./build.sh --native     # Enable native CPU optimizations
./build.sh --debug      # Build debug version
./build.sh --no-lto     # Disable link-time optimization
```

## Running Tests

```bash
cd build
ctest --output-on-failure
```

Or manually:
```bash
cd build
./tests/test_suite
```

## Installation

### System-wide Installation

```bash
cd build
sudo make install
```

This installs:
- Headers to `/usr/local/include/libloong/`
- Library to `/usr/local/lib/`
- pkg-config file to `/usr/local/lib/pkgconfig/`

### Custom Installation Prefix

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/libloong
make install
```

## Using in Your Project

### Method 1: System Installation + pkg-config

```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(libloong REQUIRED IMPORTED_TARGET libloong)

add_executable(myprogram main.cpp)
target_link_libraries(myprogram PkgConfig::libloong)
```

### Method 2: Add as Subdirectory

```cmake
add_subdirectory(path/to/libloong/lib)
target_link_libraries(myprogram loongarch)
```

### Method 3: FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(libloong
  GIT_REPOSITORY https://github.com/your-org/libloong
  GIT_TAG main
)
FetchContent_MakeAvailable(libloong)
target_link_libraries(myprogram loongarch)
```

## Cross-Compilation

### For LoongArch Host

```bash
cmake .. \
  -DCMAKE_C_COMPILER=loongarch64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=loongarch64-linux-gnu-g++
make -j$(nproc)
```

## Troubleshooting

### CMake Can't Find C++20 Compiler

Ensure you have a recent compiler:
```bash
g++ --version  # Should be 10.0 or later
clang++ --version  # Should be 12.0 or later
```

Update CMake cache:
```bash
rm -rf build
mkdir build && cd build
cmake ..
```

### Linker Errors

Ensure pthread is available:
```bash
sudo apt install libpthread-stubs0-dev
```

### Performance Issues

Build with optimizations and native CPU features:
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DNATIVE=ON -DLTO=ON
```

## Advanced Build Configurations

### Static Library

```bash
cmake .. -DBUILD_SHARED_LIBS=OFF
```

### Shared Library

```bash
cmake .. -DBUILD_SHARED_LIBS=ON
```

### With Sanitizers (for development)

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"
make -j$(nproc)
```

### With Binary Translation

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DLA_BINARY_TRANSLATION=ON
make -j$(nproc)
```

This enables JIT compilation for faster execution.

## Generating Documentation

```bash
# With Doxygen
doxygen Doxyfile

# View HTML documentation
xdg-open docs/html/index.html
```

## Creating Packages

### Debian Package

```bash
cd build
cpack -G DEB
sudo dpkg -i libloong-dev_*.deb
```

### RPM Package

```bash
cd build
cpack -G RPM
sudo rpm -i libloong-dev-*.rpm
```

## Benchmarking

Build optimized version:
```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DNATIVE=ON \
  -DLTO=ON \
  -DLA_BINARY_TRANSLATION=ON

make -j$(nproc)
```

Run benchmarks:
```bash
cd build
./benchmark/run_benchmarks
```
