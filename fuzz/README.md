# libloong Fuzzer

This directory contains a LibFuzzer-based fuzzer for the libloong LoongArch emulator.

## Overview

The fuzzer tests the instruction set by:
1. Taking random input data from LibFuzzer
2. Rounding it to 4-byte alignment (LoongArch instructions are 4 bytes)
3. Using `init_execute_area()` to create a fast-path execute segment
4. Executing the instructions with a limited instruction count (5,000 instructions max)

This approach is inspired by the libriscv fuzzer and helps discover bugs in instruction decoding and execution.

## Building

The fuzzer requires Clang (for LibFuzzer support) and should be built separately from the main project:

```bash
cd fuzz
mkdir -p build && cd build
cmake -DCMAKE_CXX_COMPILER=clang++-18 -DCMAKE_C_COMPILER=clang-18 ..
make
```

**Note:** LibFuzzer is only available with Clang. GCC is not supported.

## Running

Basic fuzzing (runs indefinitely):
```bash
./vmfuzzer
```

Limited runs:
```bash
./vmfuzzer -runs=10000
```

With custom max length:
```bash
./vmfuzzer -runs=10000 -max_len=256
```

With a corpus directory:
```bash
mkdir corpus
./vmfuzzer corpus/ -runs=10000
```

## Sanitizers

The fuzzer is built with AddressSanitizer and UndefinedBehaviorSanitizer by default. You can change this in [CMakeLists.txt](CMakeLists.txt) by modifying the `FUZZER_MODE` variable:

```cmake
# Current: address,undefined
# Alternative: memory,undefined (MemorySanitizer)
set(FUZZER_MODE "address,undefined")
```

## Coverage

The fuzzer generates coverage information. To view coverage reports, use:

```bash
# After running the fuzzer
llvm-cov show vmfuzzer -instr-profile=default.profraw
```

## What Gets Tested

The fuzzer specifically tests:
- Instruction decoding (all instruction formats)
- Bytecode translation and optimization
- Instruction execution handlers
- Edge cases in instruction operands
- Memory protection and bounds checking
- Syscall handling (basic exit syscall)

## Expected Output

You'll see output like:
```
#100	NEW    cov: 1271 ft: 1492 corp: 10/37b lim: 4 exec/s: 0 rss: 38Mb
```

Where:
- `cov`: Code coverage (edges covered)
- `ft`: Features (unique execution paths)
- `corp`: Corpus size (number of interesting inputs / total bytes)
- `lim`: Current input length limit
- `exec/s`: Executions per second
- `rss`: Memory usage

## Common Messages

**UNIMPLEMENTED**: Indicates the fuzzer found an unimplemented instruction. This is expected and not a bug.

**Protection fault**: Indicates an instruction tried to access invalid memory. This is caught and handled.

**Machine exception**: Indicates an instruction triggered an exception. This is expected during fuzzing.

## Tips

1. Start with small runs (100-1000) to verify everything works
2. Use a corpus directory to preserve interesting inputs
3. For long fuzzing sessions, use `-max_total_time=3600` (1 hour)
4. Monitor memory usage with the `rss` field
5. Save crashes with `-artifact_prefix=crashes/`

## Advanced Usage

Find crashes and save them:
```bash
mkdir crashes
./vmfuzzer -artifact_prefix=crashes/ -runs=100000
```

Resume from a corpus:
```bash
./vmfuzzer corpus/ -runs=100000
```

Minimize a crashing input:
```bash
./vmfuzzer -minimize_crash=1 crash-file
```
