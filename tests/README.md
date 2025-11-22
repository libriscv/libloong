# LoongArch Debugger

A debugging and instruction tracing tool for LoongArch binaries.

## Overview

The `debug_test` program executes LoongArch ELF binaries with detailed instruction-by-instruction tracing, register state monitoring, and optional objdump comparison. This tool is invaluable for debugging emulator behavior and understanding program execution.

## Building

From the project root:

```bash
mkdir build && cd build
cmake ..
make
```

The debugger binary will be located at `build/tests/debug_test`.

## Usage

```
debug_test [options] <binary>
```

| Option | Long Form | Description |
|--------|-----------|-------------|
| `-h` | `--help` | Show help message |
| `-i <num>` | `--max-instructions` | Maximum instructions to execute (default: 10000000) |
| `-m <size>` | `--memory` | Maximum memory in MiB (default: 256) |
| `-q` | `--quiet` | Disable verbose loader and syscalls |
| `-r` | `--registers` | Show register state after each instruction |
| `-o` | `--compare-objdump` | Compare execution with objdump disassembly |
| `-s` | `--short` | Use short output format |

### Examples

**Basic execution with full tracing:**
```bash
./debug_test program.elf
```

**Limit execution to 1 million instructions with short output:**
```bash
./debug_test --max-instructions 1000000 --short program.elf
```

**Quiet mode with hidden registers:**
```bash
./debug_test -q -r program.elf
```

**Compare with objdump disassembly:**
```bash
./debug_test --compare-objdump program.elf
```

**Allocate more memory:**
```bash
./debug_test --memory 512 program.elf
```

## Output Format

The debugger provides detailed execution traces:

```
Starting execution at PC=0x120000790

PC=0x120000790  addi.d  $sp, $sp, -48     ($sp = 0x40003ff0)
  $sp = 0x40003ff0
PC=0x120000794  st.d    $ra, $sp, 40      (MEM[0x40004018] = 0x0)
  $ra = 0x0
```

Each instruction shows:
- Program counter (PC)
- Disassembly with operands
- Computed result (for relevant instructions)
- Affected register values

## Environment Variables (Windows/Non-POSIX Fallback)

On platforms without `getopt_long` support, configuration can be done via environment variables:

| Variable | Description |
|----------|-------------|
| `MAX_INSTRUCTIONS` | Maximum instructions to execute |
| `MEMORY_MAX` | Maximum memory in MiB |
| `QUIET` | Set to disable verbose output |
| `VERBOSE_REGISTERS` | Set to "0" to hide registers |
| `COMPARE_OBJDUMP` | Set to enable objdump comparison |
| `SHORT` | Set to enable short output format |

**Example (Windows):**
```cmd
set MAX_INSTRUCTIONS=1000000
set SHORT=1
debug_test.exe program.elf
```

**Example (Unix):**
```bash
QUIET=1 SHORT=1 ./debug_test program.elf
```

## Use Cases

### Emulator Development
Verify instruction decoder and execution correctness:
```bash
./debug_test --compare-objdump test_program.elf
```

### Performance Analysis
Count instructions for specific programs:
```bash
./debug_test --max-instructions 0 --short benchmark.elf
```

### Debugging Guest Programs
Trace execution to find bugs in LoongArch binaries:
```bash
./debug_test --verbose program.elf 2>&1 | less
```

### Regression Testing
Automate execution verification:
```bash
./debug_test --quiet --short test.elf && echo "PASS" || echo "FAIL"
```

## Integration with CI

The debugger is used in continuous integration for instruction validation. See the GitHub Actions workflows for automated testing.

## Troubleshooting

### "Failed to open file"
Ensure the binary path is correct and the file exists.

### "Machine exception"
The guest program triggered an exception (invalid memory access, illegal instruction, etc.). Check the PC and register state in the error output.

### "Execution timeout"
The program exceeded the instruction limit. Increase with `--max-instructions` or use `0` for unlimited execution.

### Missing objdump
If using `--compare-objdump`, ensure `loongarch64-linux-gnu-objdump` is installed:
```bash
sudo apt-get install binutils-loongarch64-linux-gnu
```

## Architecture

The debugger wraps `Machine<LA64>` with `DebugMachine<LA64>`, which:
1. Hooks into instruction execution
2. Disassembles each instruction
3. Tracks register changes
4. Formats output for human readability

This is built on top of the core emulation library, providing a debugging layer without modifying the fast path.

## See Also

- [Unit Tests](unit/README.md) - Automated testing framework
- [Emulator CLI](../emulator/README.md) - Production emulator
- [API Documentation](../docs/API.md) - Library integration guide
