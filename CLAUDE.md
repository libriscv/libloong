# Project information

This project is a custom high-performance LoongArch emulator with a flat memory arena as memory instead of virtual paging, designed otherwise in the same way as libriscv. Static LoongArch ELFs are loaded, executed and after execution one can vmcall functions in the guest.

## Project structure

It's a CMake project with this structure:
- /lib/libloong has the C++ library implementation
    - Instruction templates are in /lib/libloong/la_instr_impl.hpp and /lib/libloong/la_instr_atomic.hpp
	- Instruction printers (for debugging) is in /lib/libloong/la_instr_printers.hpp
	- Instruction decoding is in /lib/libloong/la64.cpp
- /tests/unit has the unit tests (and a run_unit_tests.sh)
    - Unit tests build temporary C and C++ programs and execute them in various ways in the emulator
- /emulator is a C++ CLI with build.sh producing .build/laemu
    - laemu runs forever (or until completion) unless -f <instruction count> is passed
	- it uses a different, faster dispatch than the debugger
- /tests/debug_test.cpp is a full-fledged CLI debugger

## How to debug

`./tests/debug_test` is a comprehensive debugger built in the build folder. It prints each instruction as it executes. Passing -r will also show register state *after* each instruction. Passing -o will compare against ground-truth objdump in case an instruction is incorrectly decoded.

### Examples

Execute `return_42_bare` until completion, compare instructions (left) against objdump (right):
```sh
build$ ./tests/debug_test -o tests/loongarch_bins/return_42_bare
```

Execute `return_42_bare` until completion, show registers after each instruction, compare instructions (left) against objdump (right):
```sh
build$ ./tests/debug_test -r -o tests/loongarch_bins/return_42_bare
```

Execute `cxx_test` until completion without debug lines, then start executing the guest-side function `test_exception` with debugging. This saves a bunch of time and prevents logspam. Only debug lines from test_exception will be shown.
```sh
build$ ./tests/debug_test -o tests/loongarch_bins/cxx_test --call test_exception
```

## Adding a new bytecode

A single bytecode means modifying 5 different places:
1. Adding the new bytecode in threaded_bytecodes.hpp
2. Decoding to the new bytecode in decoder_cache.cpp
3. Implementing an optimized instruction layout in threaded_rewriter.cpp
4. Implementing the actual bytecode handler in bytecode_impl.cpp
5. Adding an entry connecting the bytecode to the handler in threaded_bytecode_array.hpp

Only once all 5 steps are complete can the CLI be tested. Bytecodes should be implemented according to popularity as LoongArch has very many instructions. Any bytecode where register zero could be written to can be rewritten in the optimizer to either NOP or INVALID (bytecode==0) when rd == 0, so that a pointless check for rd != 0 is avoided in hot-path.

Many instructions have slow-path instructions already decoded in la64.cpp and implemented in la_instr_impl.hpp, which can be used as a starting point. We generally don't implement any atomic instructions as bytecodes, as they just occupy instruction space.

## Instruction statistics

Running programs in the CLI with --stats will execute the guest program to completion and show statistics about which instructions have been decoded into bytecodes and not. Any instruction that has the bytecode FUNCTION is using the slow-path and is not really implemented as a bytecode, rather a fallback that calls the slow-path instruction handler is used. Popular instructions need to have a dedicated optimized bytecode for good emulation performance. If an instruction is showing as both FUNCTION and a dedicated bytecode, it is guaranteed that the bytecode decoding step is not covering the instruction properly.

## Testing long-running programs

Some programs runs for so long that only the emulator CLI can run it properly:

```sh
emulator$ ./build.sh && .build/laemu ../tests/programs/coremark.elf
```

Hard to debug with, but can be a fast way to check if the emulator is still sane and working.

## Key Technical Concepts

- **Threaded dispatch**: Using computed goto (GCC/Clang extension) for fast bytecode dispatch
- **Block-based execution**: Pre-computing block boundaries to execute multiple instructions between divergence points
- **Bytecode translation**: Converting instructions to bytecodes during cache population (and optimizing them)
- **PC management**: Program Counter handling for control flow
- **Instruction counting**: Tracking executed instructions for timeouts
- **Handler caching**: Storing index to slow-path instruction handlers in decoder cache
- **Build-time dispatch selection**: Conditional compilation based on compiler
- **Decoder cache**: Pre-decoded instruction information stored per 4-byte instruction
- **Diverging vs non-diverging instructions**: PC-modifying vs regular instructions

## Avoid these pitfalls

These are directions to avoid when investigating:

1. BSS is always zeroed. The emulators memory is backed by an anonymous memory mapping and is always zeroes when starting. glibc is also likely to zero BSS.
2. TLS is handled fully and entirely by glibc in the guest. We do not need to deal with thread-local storage at all.
3. glibc resolves IFUNCs in the guest. We do not need to care about ELF relocs.
4. LoongArch Linux ELFs will have LSX instructions in them no matter what. Trying -mno-lsx will have no effect.
5. Threaded dispatch exists only in the emulator CLI and unit tests and *not* the debugger
