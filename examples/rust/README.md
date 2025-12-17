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

## Project Structure

- `Cargo.toml` - Rust project configuration
- `.cargo/config.toml` - Build configuration for LoongArch target with custom linker settings
- `src/hello.rs` - Rust source code using std
- `build.sh` - Build script
