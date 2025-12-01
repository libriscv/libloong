# libloong Examples

This directory contains example programs demonstrating how to use libloong.

## Building the Examples

```bash
mkdir build && cd build
cmake ..
make
```

## Running the Examples

### Simple Example

Basic usage of the libloong library:

```bash
./simple_example
```

This example demonstrates:
- Creating a machine instance
- Setting up memory
- Installing syscalls
- Reading and writing memory
- Accessing registers

### Custom Syscall Example

Shows how to implement custom system calls:

```bash
./custom_syscall_example
```

This example demonstrates:
- Installing custom syscall handlers
- Reading guest memory
- Communicating between host and guest
- Multiple syscall handlers

## Example Descriptions

### simple.cpp

A minimal example showing basic libloong usage:
- Machine creation
- Memory configuration
- Linux syscall setup
- Memory operations
- Register access

### custom_syscall.cpp

Advanced example with custom syscalls:
- Custom syscall installation
- Guest-to-host communication
- Memory string operations
- Multiple syscall handlers

### rust/

Rust language example for LoongArch:
- Cross-compilation to LoongArch
- Static linking with Rust std
- Fibonacci and basic algorithms
- String and vector operations

See [rust/README.md](rust/README.md) for details.

## Using with Real LoongArch Binaries

To use these examples with actual LoongArch ELF binaries:

1. Compile a LoongArch program:
```bash
loongarch64-linux-gnu-gcc-14 -static -O2 -o hello hello.c
```

2. Modify the example to load your binary:
```cpp
std::vector<uint8_t> load_file(const char* filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    file.read((char*)buffer.data(), size);
    return buffer;
}

auto binary = load_file("hello");
Machine machine { binary };
```

3. Run the modified example with your binary.

## See Also

- [Integration Guide](../docs/INTEGRATION.md)
- [API Reference](../docs/API.md)
- [Full Examples](../docs/EXAMPLES.md)
