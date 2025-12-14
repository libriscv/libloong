# Embedded Binary Translation - Complete Example

This document shows a complete end-to-end example of using embedded binary translation.

## Prerequisites

- LoongArch cross-compiler (`loongarch64-linux-gnu-gcc`)
- CMake 3.9.4 or later
- Make

## Step-by-Step Example

### 1. Create a Simple LoongArch Program

First, create a simple test program:

```c
// hello.c
#include <stdio.h>

int main() {
    printf("Hello from LoongArch!\n");
    return 42;
}
```

### 2. Compile for LoongArch

Compile it as a static 64-bit LoongArch ELF:

```bash
loongarch64-linux-gnu-gcc -static -O2 \
    -Wl,-Ttext-segment=0x200000 \
    -o hello.elf hello.c
```

**Note**: The `-Wl,-Ttext-segment=0x200000` flag is required because libloong's memory starts at zero.

### 3. Build the Emulator with Binary Translation

```bash
cd emulator
./build.sh --bintr
```

This creates `.build/laemu` with binary translation support.

### 4. Generate Embedded Translation

Run the emulator with the `-O` and `--enable-embedded` flags to generate embeddable C code:

```bash
.build/laemu hello.elf -O hello_bintr.c --enable-embedded
```

**Note**: The `--enable-embedded` flag is required to include the self-registration code. Without it, the generated code is suitable for JIT compilation but not for embedding.

The generated `hello_bintr.c` will contain:
- Translated instruction handlers
- Mapping tables
- Self-registration code
- CRC32-C hash: `0xXXXXXXXX` (calculated from execute segment)

**Example output:**
```
libloong: Binary translation summary:
  - Translated 1234 instructions across 42 blocks
  - Generated 156 function mappings
  - Execute segment: 0x200000 - 0x250000 (327680 bytes)
  - Global jump targets: 89
  - Trace enabled: no
  - CRC32-C hash: 0x12345678
```

### 5. Rebuild Emulator with Embedded Translation

Now rebuild the emulator with the embedded translation:

```bash
./build.sh --bintr --embed hello_bintr.c
```

**Build output:**
```
Building LoongArch emulator...
  Build type: Release
  Native optimization: OFF
  LTO: ON
  Debug mode: OFF
  Binary translation: ON
  Threaded dispatch: ON
  Embedded bintr: /full/path/to/hello_bintr.c
...
-- Embedding binary translation from: /full/path/to/hello_bintr.c
...
Build complete! Binary: .build/laemu
```

### 6. Run with Embedded Translation

Now run the program - it will use the embedded translation automatically:

```bash
.build/laemu --verbose hello.elf
```

**Expected output:**
```
libloong: Found embedded translation for CRC32-C 0x12345678 (version: 1.0)
libloong: Using embedded binary translation (CRC32-C: 0x12345678)
libloong: Successfully activated embedded translation
Hello from LoongArch!
[Program exited with code: 42]
```

**Notice**: No compilation time! The translation is instant because it's embedded.

### 7. Compare Performance

Let's compare embedded translation vs JIT compilation:

**Without embedded translation** (JIT compilation):
```bash
# Build without embedded translation
./build.sh --bintr

# Run (will JIT compile)
time .build/laemu hello.elf
```

**Output:**
```
Hello from LoongArch!

real    0m0.150s  # Includes JIT compilation time
user    0m0.140s
sys     0m0.010s
```

**With embedded translation**:
```bash
# Build with embedded translation
./build.sh --bintr --embed hello_bintr.c

# Run (instant activation)
time .build/laemu hello.elf
```

**Output:**
```
Hello from LoongArch!

real    0m0.005s  # No compilation overhead!
user    0m0.003s
sys     0m0.002s
```

**Result**: ~30x faster startup with embedded translation!

## Advanced: Multiple Programs

You can maintain a library of embedded translations:

### Generate translations for multiple programs

```bash
# Build emulator with bintr
cd emulator
./build.sh --bintr

# Generate embedded translations (use --enable-embedded for embedding)
.build/laemu program1.elf -O translations/program1_bintr.c --enable-embedded
.build/laemu program2.elf -O translations/program2_bintr.c --enable-embedded
.build/laemu program3.elf -O translations/program3_bintr.c --enable-embedded
```

### Embed all translations

Modify `emulator/CMakeLists.txt`:

```cmake
set(SOURCES
    src/main.cpp
)

# Add all embedded translations
if (EMBED_ALL_TRANSLATIONS)
    list(APPEND SOURCES
        translations/program1_bintr.c
        translations/program2_bintr.c
        translations/program3_bintr.c
    )
    # Mark all as C++
    set_source_files_properties(
        translations/program1_bintr.c
        translations/program2_bintr.c
        translations/program3_bintr.c
        PROPERTIES LANGUAGE CXX
    )
endif()
```

Build with all translations:

```bash
cmake -DEMBED_ALL_TRANSLATIONS=ON ..
make
```

Now the emulator can instantly run any of the three programs!

## Troubleshooting

### "No embedded translation found"

**Problem:**
```
libloong: No embedded translation found for CRC32-C 0x87654321
```

**Solutions:**
1. Ensure you're running the exact same ELF file you translated
2. Check the CRC32-C hash matches (use `--verbose` on both runs)
3. Verify the embedded translation was included in the build

### "Embedded bintr file not found"

**Problem:**
```
Error: Embedded bintr file not found: hello_bintr.c
```

**Solutions:**
1. Use absolute path: `./build.sh --embed /full/path/to/hello_bintr.c`
2. Or use relative path from emulator directory: `./build.sh --embed ./hello_bintr.c`
3. Verify the file exists: `ls -l hello_bintr.c`

### Build errors with embedded file

**Problem:**
```
error: 'loongarch_register_embedded_translation' was not declared
```

**Solutions:**
1. Ensure binary translation is enabled: `--bintr`
2. Rebuild from clean state: `rm -rf .build && ./build.sh --bintr --embed file.c`

## Performance Tips

### 1. Use Native Optimization

```bash
./build.sh --bintr --native --embed hello_bintr.c
```

This enables `-march=native` for best performance on your CPU.

### 2. Generate with Optimizations

Generate translations from optimized programs:

```bash
# Compile with -O3
loongarch64-linux-gnu-gcc -static -O3 \
    -Wl,-Ttext-segment=0x200000 \
    -o hello_opt.elf hello.c

# Generate translation
.build/laemu hello_opt.elf -O hello_bintr.c
```

### 3. Profile-Guided Translation

For best results, run the program once to ensure hot paths are translated:

```bash
# Run once to warm up translation
.build/laemu --stats program.elf

# Check statistics to see coverage
# Then generate embedded translation
.build/laemu program.elf -O program_bintr.c
```

## Integration with Your Application

If you're embedding libloong in a custom application:

```cpp
// my_app.cpp
#include <libloong/machine.hpp>

// The embedded translation(s) will self-register via global constructors
// No manual registration needed!

int main() {
    auto binary = load_file("program.elf");

    loongarch::MachineOptions opts;
    opts.translate_enabled = true; // Enable binary translation

    loongarch::Machine machine(binary, opts);

    // Will automatically use embedded translation if available
    // Otherwise falls back to JIT compilation
    machine.simulate();

    return 0;
}
```

Build your application with the embedded translation:

```bash
g++ -std=c++20 -O2 my_app.cpp program_bintr.c -lloong -o my_app
./my_app  # Instant startup!
```

## See Also

- [Embedded Binary Translation Guide](EMBEDDED_BINTR.md)
- [Binary Translation Documentation](../README.md)
- [Integration Guide](INTEGRATION.md)
