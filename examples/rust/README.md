# Rust LoongArch Example

This directory contains a Rust example program for LoongArch that can be compiled and run with libloong.

## Prerequisites

Install the LoongArch Rust target and ensure you have the LoongArch GCC toolchain:
```bash
rustup target add loongarch64-unknown-linux-gnu
```

## Building

```bash
chmod +x build.sh
./build.sh
```

This will produce `hello-std.elf`, a statically-linked LoongArch binary.

## Running

### With the emulator CLI:
```bash
../../emulator/.build/laemu hello-std.elf
```

### With the debugger:
```bash
../../build/tests/debug_test hello-std.elf
```

**Note**: The example currently builds successfully but encounters runtime issues when executed in libloong due to glibc static initialization. This is a known limitation being investigated.

## What This Example Demonstrates

- **Rust compilation for LoongArch**: Cross-compilation using `loongarch64-unknown-linux-gnu` target
- **Static linking**: Fully static binary with glibc
- **Standard library usage**: Uses Rust std with println!, Vec, format!, etc.
- **Memory layout**: Configured with `-Wl,-Ttext-segment=0x200000` for libloong compatibility
- **Fibonacci computation**: Recursive algorithm demonstration
- **String and vector operations**: Standard Rust data structures

## Project Structure

- `Cargo.toml` - Rust project configuration
- `.cargo/config.toml` - Build configuration for LoongArch target with custom linker settings
- `src/hello.rs` - Rust source code using std
- `build.sh` - Build script

## Known Issues

- Static glibc binaries currently experience runtime issues in libloong
- This appears to be related to glibc's TLS/memory initialization
- Investigation ongoing for proper glibc support

## Future Work

- Add `no_std` bare-metal example with direct syscalls
- Test with musl libc as an alternative to glibc
- Add examples demonstrating LoongArch SIMD (LSX/LASX) via intrinsics
- Add more complex algorithmic examples

## Notes

- The `-Ttext-segment=0x200000` linker flag is required because libloong's memory starts at zero
- Static linking is enforced via `target-feature=+crt-static` and `-static`
- Uses `loongarch64-linux-gnu-gcc-14` as the linker
- Binary is built with optimizations enabled (`opt-level = 2`)
