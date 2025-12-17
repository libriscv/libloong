# libloong Installed Package Example

This example demonstrates using libloong after it has been installed on the system.

## Prerequisites

libloong must be installed on your system. This can be done via:
- Installing a DEB package: `sudo dpkg -i libloong-dev_*.deb`
- Installing via your package manager
- Building and installing from source: `cmake .. && make && sudo make install`

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Running

The example requires a LoongArch64 ELF binary to execute:

```bash
./installed_example <path-to-loongarch64-elf>
```

For example, if test binaries are installed:
```bash
./installed_example /usr/share/libloong/tests/return_42_bare.elf
```

## What This Example Does

1. Loads a LoongArch64 ELF binary from disk
2. Creates a libloong Machine instance with 16MB of memory
3. Sets up Linux syscalls and program arguments
4. Executes the program (with a 10 million instruction limit)
5. Reports the exit code and instruction count

This demonstrates that libloong has been correctly installed and can be used as a system library.
