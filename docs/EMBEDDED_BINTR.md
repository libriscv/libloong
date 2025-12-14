# Embedded Binary Translation

This document describes how to use embedded binary translations in libloong.

> **Key Insight**: Embedded binary translations provide **~75% of native performance** compared to JIT's **~40%**. The CLI **always activates** embedded translations when available - no special flags needed. JIT is only used as a fallback when no embedded translation exists.

## Overview

Embedded binary translation allows you to pre-compile binary translations and embed them directly into your host application. This provides:

- **Near-native performance**: ~75% of native speed (vs ~40% with JIT)
- **Zero startup overhead**: No JIT compilation needed at runtime
- **Always activated**: Embedded translations are used automatically when available in the CLI
- **Deterministic performance**: Translation happens at build time
- **Deployment simplicity**: Single binary with embedded translations
- **Cross-compilation friendly**: Translate on one architecture, run on another

## How It Works

1. **Generation**: The emulator generates C code with embedded translation metadata
2. **Registration**: Global constructors automatically register translations using CRC32-C hashes
3. **Loading**: When loading an ELF, libloong **always** checks for embedded translations first
4. **Activation**: If found, uses the pre-compiled translation immediately (instant, near-native performance)
5. **Fallback**: If not found, falls back to JIT compilation (libtcc, if enabled)

## How Code Generation Works

When you use the `-O <file>` flag, the emulator:

1. **Generates embedded C code** written to the file with:
   - All translated instruction handlers
   - Mapping tables
   - Self-registration code (global constructor)
   - `init()` function is `static` (internal linkage)

2. **Also performs JIT compilation** for immediate execution (if libtcc is available)

The generated file is specifically designed for **embedding** into your application binary, providing ~75% of native performance when activated.

**This guide focuses on using the generated embedded code.**

## Quick Start

### Step 1: Generate Embedded Translation

Use the emulator CLI to generate embeddable C code:

```bash
cd emulator
./build.sh

# Generate embedded translation for your program
.build/laemu program.elf -O program_bintr.c

# OR: Generate with unsafe optimizations (fastest, skips bounds checks)
.build/laemu program.elf --fast -O program_bintr.c
```

The generated `program_bintr.c` will include:
- All translated instruction handlers
- Mapping tables
- Self-registration code with CRC32-C hash

**Note**: Using `--fast` generates code without memory bounds checks, maximizing performance but requiring trusted guest programs.

### Step 2: Build With Embedded Translation

Use the `--embed` flag to compile the translation into your application:

```bash
./build.sh --embed program_bintr.c
```

This will:
1. Compile `program_bintr.c` as part of the emulator
2. Link it with the emulator binary
3. Enable binary translation support

### Step 3: Run Your Program

Simply run the emulator with your ELF:

```bash
.build/laemu program.elf
```

The emulator will:
1. Load the ELF file
2. Calculate CRC32-C hash of execute segment
3. Check embedded translation registry (finds your translation!)
4. **Activate it immediately** (no compilation needed, ~75% native performance)

**Note**: Embedded translations are **always** used when available in the CLI. There's no flag needed to enable them - they activate automatically because they provide significantly better performance (~75% of native) compared to JIT compilation (~40% of native).

To disable binary translation entirely (forcing bytecode interpretation), use:
```bash
.build/laemu program.elf --no-translate
```

## Advanced Usage

### Multiple Embedded Translations

You can embed multiple translations by generating multiple C files and including them all:

```bash
# Generate translations
laemu program1.elf -O program1_bintr.c
laemu program2.elf -O program2_bintr.c

# Build with both (requires manual CMake configuration)
# Add both files to emulator/CMakeLists.txt SOURCES
```

### Integration in Your Application

If you're embedding libloong in your own application:

```cpp
#include <libloong/machine.hpp>

// The embedded translation will self-register via global constructor
// Just use the library normally:

auto binary = load_file("program.elf");
loongarch::Machine machine(binary);

// Embedded translations are ALWAYS checked first and activated if found.
// This provides ~75% native performance vs ~40% with JIT.
// Only if no embedded translation exists will it fall back to JIT (if enabled).
machine.simulate();
```

### Checking If Embedded Translation Was Used

Enable verbose logging to see which translation method was used:

```bash
.build/laemu --verbose program.elf
```

Output will show:
```
libloong: Found embedded translation for CRC32-C 0x12345678 (version: 1.0)
libloong: Using embedded binary translation (CRC32-C: 0x12345678)
```

## Technical Details

### CRC32-C Hashing

The embedded translation system uses CRC32-C (Castagnoli polynomial) to identify execute segments:

- **Hash includes**: Only the execute segment bytes
- **Hardware acceleration**: Uses SSE4.2 (x86), NEON (ARM), or CRC (LoongArch) instructions
- **Collision handling**: If a collision occurs, the first registered translation wins

### Registry Limits

- **Maximum translations**: 16 embedded translations
- **Storage**: Zero-initialized BSS array (no global constructors)
- **Thread safety**: Atomic operations during registration

### Generated Code Structure

The generated C code is compatible with both C and C++ compilers. In the emulator build system, it's automatically compiled as C++ to avoid needing a separate C compiler. The code includes:

```c
// 1. Header with type definitions and API
#include <stdint.h>
// ... type definitions ...

// 2. Translated instruction handlers
static bintr_block_returns block_0x200000(CPU* cpu, ...) {
    // ... translated instructions ...
}

// 3. Mapping tables
const struct Mapping mappings[] = {
    {0x200000, 0},  // PC -> handler index
    // ...
};

// 4. Embedded dylib structure
static struct EmbeddedDylib embedded_dylib = {
    &init,
    &no_mappings,
    &mappings[0],
    &no_handlers,
    &unique_mappings[0]
};

// 5. Init function
static void* embedded_init(void) {
    return (void*)&embedded_dylib;
}

// 6. Self-registration
__attribute__((constructor))
static void register_this_translation(void) {
    loongarch_register_embedded_translation(0x12345678, embedded_init, "1.0");
}
```

### Comparison with JIT Compilation

| Feature | Embedded | JIT (libtcc) |
|---------|----------|--------------|
| **Performance** | **~75% of native** | **~40% of native** |
| Startup time | Instant | ~50-500ms |
| Memory usage | Code section | Heap allocation |
| Cross-compilation | Supported | Same arch only |
| Runtime dependencies | None | libtcc |
| Code generation | Build time | Runtime |
| **CLI activation** | **Always (when available)** | **Fallback only** |

## Troubleshooting

### Translation Not Found

If you see "No embedded translation found for CRC32-C 0x...", check:

1. **Correct ELF**: Are you running the same ELF you translated?
2. **CRC32-C match**: The execute segment must be identical
3. **Registration**: Global constructor must have run (check with `--verbose`)

### Multiple Translations for Same Hash

If multiple translations have the same CRC32-C hash (collision):
- The first registered translation will be used
- Consider adding version info or manual checks

### Build Errors

If you get compilation errors when embedding:

1. **File exists**: Verify the embedded file path is correct
2. **Out-of-date**: The file may be outdated, write it again using `-O file.c`
3. **CMake cache**: Try `rm -rf .build` and rebuild

## Performance Tips

1. **Use embedded translations**: They provide ~75% of native performance vs JIT's ~40%
2. **Use `--fast` flag**: Generate translations with `--fast -O file.c` to skip memory bounds checks (for trusted programs)
3. **Use native optimization**: Build with `--native` for best performance
4. **Enable LTO**: Link-time optimization can inline across translation units
5. **Profile-guided optimization**: Generate translation after profiling your workload
6. **Always embed critical programs**: Since embedded translations activate automatically, embedding frequently-used programs provides consistent high performance

## See Also

- [Binary Translation Documentation](BINARY_TRANSLATION.md) (if exists)
- [Integration Guide](INTEGRATION.md)
- [API Documentation](API.md)
