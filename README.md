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
Copy:            4635.9     0.038666     0.034513     0.046993
Scale:           2513.6     0.080806     0.063653     0.085414
Add:             6428.1     0.040918     0.037336     0.048907
Triad:           4948.8     0.049776     0.048497     0.057069

CoreMark 1.0 : 1894.643546 / GCC14.2.0 -O3 -DPERFORMANCE_RUN=1   / Static
```
Despite not having a fast threaded dispatch yet, it is quite fast.

## Documentation

- [Integration Guide](docs/INTEGRATION.md)
- [API Reference](docs/API.md)
- [LoongArch ISA Support](docs/ISA.md)
