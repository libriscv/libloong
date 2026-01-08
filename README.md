# libloong: High-Performance 64-bit LoongArch Emulator

**libloong** is a high-performance LoongArch (LA64) userspace emulator library designed for embedding and high-frequency scripting. Built on the architecture of [libriscv](https://github.com/libriscv/libriscv), it provides the fastest 64-bit interpreter performance available, maintaining a compact ~18k LOC codebase with zero dependencies.

You can also run LoongArch64 programs in the [command-line interface](/emulator).

## The Interpreter Advantage

While many runtimes rely on complex, multi-tiered JIT compilers to achieve performance, **libloong** focuses on maximizing interpreter efficiency. This design choice offers advantages:

* **Universal Portability:** Unlike JITs, which require platform-specific code generation and often fail on locked-down platforms, libloong runs anywhere C++20 is supported. This includes **Nintendo Switch (1 & 2)**, iOS, and other environments where JIT-compiled memory execution is restricted or forbidden.
* **Rapid Iteration:** High-performance interpretation allows developers to update guest logic instantly without re-publishing or re-signing binaries. You get the flexibility of a script with the performance of a high-end VM.
* **Ultra-Low Latency:** Traditional VMs like Lua or Java often suffer from high call overhead (~150ns). libloong achieves a **~4ns native-to-guest call overhead**, making it suitable for hot-loop game engine scripting.

## Performance Benchmarks

In **CoreMark 1.0** interpreter benchmarks (December 2025), `libloong` leads the 64-bit category on the Ryzen 7950X, reliably reaching a score of **3000+**.

### CoreMark 1.0 Interpreter Scores
| Interpreter | Architecture | Score |
| :--- | :--- | :--- |
| libloong | 64-bit LoongArch | **3045** |
| libriscv | 64-bit RISC-V | 2865 |
| stitch | Wasm | 2743 |
| wasm3 | Wasm | 2368 |
| wasmer (WAMR) | Wasm | 2314 |
| wasmi | Wasm | 1967 |

> **Advanced Modes:** While the interpreter is the primary focus for portability, libloong includes a lightweight JIT reaching **38% of native** (15.5k CoreMark) and an embedded binary translator reaching **~77% of native** (31.9k CoreMark).


## Features

* **Vector Support:** Full support for LSX and LASX instruction sets.
* **Multi-Language:** Native C++ API with [Rust](/rust) and [Go](/go) bindings.
* **Memory Safety:** Strict memory sandboxing with optional masked memory arenas.
* **State Management:** First-class support for pausing, resuming, and serializing machine state.
* **Dynamic execution**: Can load and execute dynamic executables, as well as embedded JITs (eg. LuaJIT)*

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

## Documentation

- [Integration Guide](docs/INTEGRATION.md)
- [API Reference](docs/API.md)
- [LoongArch ISA Support](docs/ISA.md)
