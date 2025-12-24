# 64-bit LoongArch emulator library

A high-performance LoongArch userspace emulator library designed for embedding and scripting applications.

Built on the proven architecture of [libriscv](https://github.com/libriscv/libriscv), libloong has competitive interpreter performance while maintaining a compact ~18k line codebase.

For discussions & help, [visit Discord](https://discord.gg/n4GcXr66X5).

## Features

- Fast LoongArch interpreter with optional JIT
- Ultra-low latency call overheads
- Support for 64-bit LoongArch (LA64)
- Support for vector LSX and LASX instructions
- C++ API with [Rust](/rust) and [Go](/go) bindings
- Zero dependencies
- Execution timeout and memory safety
- First-class pause/resume support

## Design

Game engine scripting is where libloong excels. Traditional games expose modding through shared libraries (full system access), embedded VMs like Lua (~150ns call overhead), or Java run-times. _libloong_ has ~4ns call overhead.

![t](https://github.com/user-attachments/assets/6a399ceb-b092-45a2-b299-5d272f95ac35)

See the [example Asteroid game](/examples/gamedev).

## Building

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
make -j6
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

## Performance

STREAM memory benchmark:
```sh
Function    Best Rate MB/s  Avg time     Min time     Max time
Copy:           33146.7     0.004884     0.004827     0.004962
Scale:          27825.2     0.005792     0.005750     0.005920
Add:            31388.6     0.007712     0.007646     0.007797
Triad:          29250.7     0.008268     0.008205     0.008379
```
There is a also a STREAM-like benchmark [written in Rust](/examples/rust) in the examples:
```sh
Fill 76.3 MiB rate 27.9 GB/s | time min 2.9ms avg 3.1ms max 3.3ms
Copy 153 MiB  rate 35.3 GB/s | time min 4.5ms avg 4.6ms max 5.0ms
Scale 153 MiB rate 23.0 GB/s | time min 7.0ms avg 7.0ms max 7.1ms
Add 229 MiB   rate 31.9 GB/s | time min 7.5ms avg 7.6ms max 7.7ms
Triad 229 MiB rate 11.1 GB/s | time min 21.5ms avg 21.6ms max 21.8ms
```

<img width="600" height="371" alt="CoreMark 1 0 interpreters, Dec 2025 (Ryzen 7950X)" src="https://github.com/user-attachments/assets/b37e985e-8332-44fc-880b-781bc1a07cc5" />

Register machines still stand strongest at the end of 2025. _libloong_ is currently the fastest 64-bit interpreter, reliably reaching 3000+ CoreMark score.

The lightweight JIT reaches 38% of native performance (15.5k vs 41k CoreMark) with full feature parity to the interpreter:

> CoreMark 1.0 : 15580.375613 / GCC14.2.0 -O3 -DPERFORMANCE_RUN=1   / Static

Using embedded binary translation, it's currently possible to reach ~77% of native:

> CoreMark 1.0 : 31962.238533 / GCC14.2.0 -O3 -DPERFORMANCE_RUN=1   / Static

.. however more work is needed to reach full potential. The upper bound for embedded binary translation should be around ~90% of native.

## Documentation

- [Integration Guide](docs/INTEGRATION.md)
- [API Reference](docs/API.md)
- [LoongArch ISA Support](docs/ISA.md)
