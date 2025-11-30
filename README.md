# libloong

A LoongArch userspace emulator library, based on the design and architecture of [libriscv](https://github.com/libriscv/libriscv).

It's currently a work in progress, but is able to boot into main() for full Linux glibc guest programs. Hurray!

## Features

- Fast LoongArch interpreter with decoder cache
- Support for LA32 and LA64 architectures
- Support for vector LSX and LASX instructions
- C and C++ API

## Building

### Standard Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Build Options

CMake configuration options:

**Architecture Support:**
- `LA_32=ON/OFF` - Enable LA32 (32-bit) support (default: ON)
- `LA_64=ON/OFF` - Enable LA64 (64-bit) support (default: ON)

**Features:**
- `LA_DEBUG=ON/OFF` - Enable debug output (default: OFF)
- `LA_BINARY_TRANSLATION=ON/OFF` - Enable binary translation (experimental, default: OFF)
- `LA_THREADED=ON/OFF` - Enable threaded bytecode dispatch (default: ON)
- `LA_MASKED_MEMORY_BITS=N` - Set masked memory arena size to 2^N bytes (0 = disabled, default: 0)

**Example with options:**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DLA_MASKED_MEMORY_BITS=32 \
         -DLA_BINARY_TRANSLATION=ON
make -j$(nproc)
```

## Quick Start

```cpp
#include <libloong/machine.hpp>

int main() {
    // Load a LoongArch ELF binary
    std::vector<uint8_t> binary = load_file("program.elf");

    // Create a machine with 64MB memory
    loongarch::Machine<loongarch::LA64> machine { binary, {
        .memory_max = 64 * 1024 * 1024
    }};

    // Setup program arguments
    machine.setup_linux({"program"}, {"LC_ALL=C"});

    // Run the program
    machine.simulate();
}
```

## Architecture

libloong follows the same design principles as libriscv:

- **Machine**: The main emulator state, containing CPU and Memory
- **CPU**: Instruction decoder, executor, and register file
- **Memory**: Arena-based memory management with multiple execute segments
- **Decoder Cache**: Fast instruction decoding and dispatch

```sh
Function    Best Rate MB/s  Avg time     Min time     Max time
Copy:           32493.5     0.005076     0.004924     0.005207
Scale:           3932.7     0.045100     0.040685     0.049495
Add:            11422.0     0.023531     0.021012     0.030166
Triad:           9690.3     0.025947     0.024767     0.028585

CoreMark 1.0 : 2959.140192 / GCC14.2.0 -O3 -DPERFORMANCE_RUN=1   / Static
```

## Documentation

- [Integration Guide](docs/INTEGRATION.md)
- [API Reference](docs/API.md)
- [LoongArch ISA Support](docs/ISA.md)
