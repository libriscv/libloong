# libloong

A high-performance LoongArch userspace emulator library designed for embedding and scripting applications.

Built on the proven architecture of [libriscv](https://github.com/libriscv/libriscv), libloong has competitive interpreter performance while maintaining a compact ~17k line codebase.

For discussions & help, [visit Discord](https://discord.gg/n4GcXr66X5).

## Features

- Fast LoongArch interpreter with optional JIT
- Ultra-low latency call overheads
- Support for 64-bit LoongArch (LA64)
- Support for vector LSX and LASX instructions
- C and C++ API
- Zero dependencies
- Execution timeout and memory safety
- First-class pause/resume support

## Design

Game engine scripting is where libloong excels. Traditional games expose modding through shared libraries (full system access), embedded VMs like Lua (~150ns call overhead), or Java run-times. _libloong_ has ~4ns call overhead.

```sh
+==============================================================================+
|  Score: 260                                               Difficulty: 1.1    |
|                                                                              |
|                                                                              |
|                         O O                                                  |
|                                                                              |
|                                                                              |
|                                                                              |
|                                                                              |
|                                                                              |
|                                                                              |
|            .@.                                                               |
|                                                                              |
|                                                                              |
|                                                                              |
|                                                                              |
|                                                                              |
|                                                                              |
|                                  O                                           |
|                                                                              |
|                                                                              |
|                              /A\                                             |
|  A/D: Move  Q: Quit                                                          |
+= LoongScript JIT  Time: 49.28us  Instr: 2780  MI/s: 56.41 ===================+
```
See the [example Asteroid game](/examples/gamedev).

## Building

### Standard Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Build Options

CMake configuration options:

- `LA_DEBUG=ON/OFF` - Enable debug output (default: OFF)
- `LA_BINARY_TRANSLATION=ON/OFF` - Enable binary translation (default: OFF)
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
    loongarch::Machine machine { binary, {
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
Copy:           32634.1     0.005007     0.004903     0.005219
Scale:          15336.0     0.012081     0.010433     0.013761
Add:            19884.1     0.015094     0.012070     0.017555
Triad:          13502.2     0.021949     0.017775     0.024309
```

<img width="600" height="371" alt="CoreMark 1 0 interpreters, Dec 2025 (Ryzen 7950X)" src="https://github.com/user-attachments/assets/b37e985e-8332-44fc-880b-781bc1a07cc5" />

Register machines still stand strongest at the end of 2025. _libloong_ is currently the fastest 64-bit interpreter, reliably reaching 3000+ CoreMark score.

The optional lightweight JIT reaches 38% of native performance (15.5k vs 41k CoreMark) with full feature parity to the interpreter:

> CoreMark 1.0 : 15580.375613 / GCC14.2.0 -O3 -DPERFORMANCE_RUN=1   / Static


## Documentation

- [Integration Guide](docs/INTEGRATION.md)
- [API Reference](docs/API.md)
- [LoongArch ISA Support](docs/ISA.md)
