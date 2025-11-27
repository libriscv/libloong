# libloong

A LoongArch userspace emulator library, based on the design and architecture of [libriscv](https://github.com/libriscv/libriscv).

It's currently a work in progress, but is able to boot into main() for full Linux glibc guest programs. Hurray!

## Features

- Fast LoongArch interpreter with decoder cache
- Support for LA32 and LA64 architectures
- Support for vector LSX and LASX instructions
- C and C++ API

## Building

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
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
- **Decoder Cache**: Fast instruction decoding and dispatch (eventually)

```sh
Function    Best Rate MB/s  Avg time     Min time     Max time
Copy:           32083.4     0.005137     0.004987     0.005475
Scale:           3723.5     0.043730     0.042970     0.045769
Add:             8703.5     0.027995     0.027575     0.028574
Triad:           5706.6     0.042222     0.042057     0.042625

CoreMark 1.0 : 2676.973192 / GCC14.2.0 -O3 -DPERFORMANCE_RUN=1   / Static
```
Despite not having a complete threaded dispatch yet, it is quite fast.

## Documentation

- [Integration Guide](docs/INTEGRATION.md)
- [API Reference](docs/API.md)
- [LoongArch ISA Support](docs/ISA.md)
