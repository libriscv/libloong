# LoongArch Emulator (laemu)

A high-performance command-line emulator for executing LoongArch ELF binaries.

## Overview

`laemu` (LoongArch Emulator) is a userspace emulator that runs LoongArch LA32 and LA64 binaries on any host platform. It provides full Linux syscall emulation, allowing native execution of statically-linked LoongArch programs.

## Building

### Standard Build

From the emulator directory:

```bash
cd emulator
mkdir -p .build
cd .build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

### Optimized Build

Enable additional optimizations:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DNATIVE=ON -DLTO=ON
make
```

**Build Options:**
- `NATIVE=ON` - Enable native CPU optimizations (`-march=native`)
- `LTO=ON` - Enable link-time optimization for better performance

The emulator binary will be located at `.build/laemu`.

## Usage

```
laemu [options] <program> [args...]
```

### Options

| Option | Long Form | Description |
|--------|-----------|-------------|
| `-h` | `--help` | Show help message |
| `-v` | `--verbose` | Enable verbose output (loader & syscalls) |
| `-s` | `--silent` | Suppress all output except errors |
| `-t` | `--timing` | Show execution timing and instruction count |
| `-f <num>` | `--fuel <num>` | Maximum instructions to execute (default: 2000000000)<br/>Use 0 for unlimited |
| `-m <size>` | `--memory <size>` | Maximum memory in MiB (default: 512) |

**Note:** The emulator automatically detects LA32/LA64 architecture from the ELF binary header.

### Examples

**Execute a program:**
```bash
./laemu program.elf
```

**Pass arguments to the guest program:**
```bash
./laemu program.elf arg1 arg2 arg3
```

**Verbose mode with timing:**
```bash
./laemu --verbose --timing program.elf
```

**Limit execution and memory:**
```bash
./laemu --fuel 1000000 --memory 256 program.elf
```

**Silent execution (only errors and exit code):**
```bash
./laemu --silent program.elf
```

**Unlimited execution:**
```bash
./laemu --fuel 0 long_running_program.elf
```

## Output

### Default Mode
```
Program exited with code 0
```

### Verbose Mode
Shows loader information and syscall traces:
```
Loaded 4096 bytes from program.elf
Detected LA64 architecture
Arguments:
  program.elf
  arg1
Program entry point at: 0x120000790
[Syscall traces...]
Program exited with code 0
```

### Timing Mode
Includes execution statistics:
```
Program exited with code 0 after 1234567 instructions (0.142857 seconds)
```

## Exit Codes

The emulator returns:
- **Guest exit code** - If the program completes successfully
- **-1** - If execution timeout or exception occurs
- **1** - If fatal error (file not found, invalid binary, etc.)

## Environment Variables (Windows/Non-POSIX Fallback)

On platforms without `getopt_long` support, configuration can be done via environment variables:

| Variable | Description |
|----------|-------------|
| `VERBOSE` | Set to enable verbose output |
| `SILENT` | Set to enable silent mode |
| `TIMING` | Set to enable timing output |
| `FUEL` | Maximum instructions to execute (0 = unlimited) |
| `MEMORY` | Maximum memory in MiB |

**Note:** Architecture is always auto-detected from the ELF binary.

**Example (Windows):**
```cmd
set TIMING=1
set MEMORY=256
laemu.exe program.elf
```

**Example (Unix):**
```bash
VERBOSE=1 TIMING=1 ./laemu program.elf
```

## Features

### Performance
- Fast interpreter with decoder cache
- Efficient memory management
- Native optimizations available via `-DNATIVE=ON`
- Link-time optimization support

### Compatibility
- Full Linux syscall emulation
- Static binary support
- LA32 and LA64 architectures
- Cross-platform (Linux, macOS, Windows)

### Execution Control
- Configurable instruction limits (fuel)
- Memory limits
- Silent mode for scripting
- Detailed timing information

### Program Arguments
Arguments passed after the program path are forwarded to the guest:
```bash
./laemu program.elf --guest-arg value
# Guest sees: argv[0]="program.elf", argv[1]="--guest-arg", argv[2]="value"
```

## Use Cases

### Running Cross-Compiled Programs

Compile and run LoongArch programs on x86_64:

```bash
loongarch64-linux-gnu-gcc -static hello.c -o hello.elf
./laemu hello.elf
```

### Performance Testing

Benchmark LoongArch binaries:

```bash
./laemu --timing benchmark.elf
```

### CI/CD Integration

Automated testing of LoongArch software:

```bash
#!/bin/bash
for test in tests/*.elf; do
  echo "Running $test..."
  ./laemu --silent "$test" && echo "PASS" || echo "FAIL"
done
```

### Development Workflow

Test cross-compiled code without LoongArch hardware:

```bash
# Build
loongarch64-linux-gnu-gcc -static myapp.c -o myapp.elf

# Test
./laemu myapp.elf test_input.txt

# Debug
./laemu --verbose myapp.elf
```

## Performance

Despite being an interpreter, laemu achieves impressive performance:

**STREAM Benchmark Results:**
```
Function    Best Rate MB/s  Avg time     Min time     Max time
Copy:            4635.9     0.038666     0.034513     0.046993
Scale:           2513.6     0.080806     0.063653     0.085414
Add:             6428.1     0.040918     0.037336     0.048907
Triad:           4948.8     0.049776     0.048497     0.057069
```

For optimal performance, build with:
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DNATIVE=ON -DLTO=ON
```

## Binary Requirements

The emulator supports:
- **Statically-linked** LoongArch ELF binaries
- **LA32** and **LA64** architectures
- Linux syscall ABI

Compile guest programs with:
```bash
# LA64 (64-bit)
loongarch64-linux-gnu-gcc -static program.c -o program.elf

# LA32 (32-bit) - if supported
loongarch32-linux-gnu-gcc -static program.c -o program32.elf
```

## Troubleshooting

### "Failed to open file"
- Check the file path
- Ensure the file exists and is readable

### "LA32/LA64 support not compiled in"
- The binary was compiled without support for that architecture
- Rebuild with the appropriate flags

### "Execution timeout"
- Program exceeded instruction limit
- Increase fuel: `--fuel 10000000000`
- Or use unlimited: `--fuel 0`

### Slow Execution
- Ensure you built in Release mode
- Try native optimizations: `-DNATIVE=ON`
- Enable LTO: `-DLTO=ON`

### "Machine exception"
- Guest program has a bug (segfault, illegal instruction, etc.)
- Try verbose mode: `--verbose`
- Or use the debugger: `../tests/debug_test program.elf`

## Comparison with Debugger

libloong provides two executables:

| Tool | Purpose | Output | Speed |
|------|---------|--------|-------|
| `laemu` | Production emulator | Minimal | Fast |
| `debug_test` | Instruction tracer | Full traces | Slow |

Use `laemu` for:
- Running programs normally
- Performance testing
- CI/CD integration
- Production workflows

Use `debug_test` for:
- Debugging guest programs
- Verifying emulator correctness
- Understanding instruction execution
- Development and testing

## Integration with CI/CD

Example GitHub Actions usage:

```yaml
- name: Build emulator
  run: |
    cd emulator
    ./build.sh

- name: Run tests
  run: |
    ./emulator/.build/laemu test_suite.elf
```

See [.github/workflows/emulator-build.yml](../.github/workflows/emulator-build.yml) for a complete example.

## Architecture

The emulator is built on libloong:

```
laemu (CLI)
    ↓
Machine<LA64/LA32>
    ↓
┌─────────────┬──────────────┐
│ CPU         │ Memory       │
│ - Decoder   │ - Arenas     │
│ - Executor  │ - Segments   │
│ - Registers │ - Page table │
└─────────────┴──────────────┘
    ↓
Linux Syscall Emulation
```

## See Also

- [Debugger](../tests/README.md) - Instruction-level debugging tool
- [Library Integration](../docs/INTEGRATION.md) - Using libloong in your project
- [API Reference](../docs/API.md) - Full library API
- [ISA Support](../docs/ISA.md) - Supported LoongArch instructions
