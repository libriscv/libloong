# libloong Go Bindings

Go bindings for **libloong**, a high-performance 64-bit LoongArch emulator with flat memory arena.

## Features

- 🚀 **Fast Execution**: Optimized bytecode dispatch and optional binary translation
- 🎯 **VMCall Support**: Call guest functions directly from Go
- 💾 **Memory Operations**: Read/write guest memory with full control
- 🔧 **Accelerated Heap**: Managed guest heap allocation (arena)
- 🐧 **Linux Environment**: Full Linux syscall emulation
- 🔒 **Type-Safe**: Idiomatic Go API with proper error handling

## Requirements

- **Go 1.21+**
- **CMake 3.15+**
- **GCC 14+** or **Clang 18+** (C++20 support required)
- **LoongArch64 cross-compiler** (for building guest programs): `loongarch64-linux-gnu-gcc`

## Installation

### Prerequisites

- **Go 1.21+**
- **CMake 3.15+** - For building the C++ library
- **GCC 14+** or **Clang 18+** - C++20 support required
- **LoongArch64 cross-compiler** (optional, for building guest programs): `loongarch64-linux-gnu-gcc`

Install build dependencies:

```bash
# Ubuntu/Debian
sudo apt-get install cmake g++-14

# macOS
brew install cmake

# Fedora/RHEL
sudo dnf install cmake gcc-c++
```

### Installation Methods

#### Option 1: From Repository (Recommended for Development)

```bash
# Clone the repository
git clone https://github.com/gonz/libloong
cd libloong/go

# Build native libraries and wrapper (automated)
go generate

# Now you can use it
go build ./examples/hello_world.go
```

#### Option 2: As a Go Module Dependency

```bash
# Add to your project
go get github.com/gonz/libloong/go

# Navigate to the package directory and build native code
cd $(go env GOPATH)/pkg/mod/github.com/gonz/libloong@*/go
go generate

# Return to your project
cd -

# Now use it in your code
```

**Note**: The `go generate` step is required **once** to build the C++ library and wrapper. This is because CGO cannot automatically build C++ projects with CMake.

### Verifying Installation

```bash
# Test that everything works
cd go
go test -v
```

## Quick Start

### Basic Program Execution

```go
package main

import (
    "fmt"
    "os"

    "github.com/gonz/libloong/go"
)

func main() {
    // Read LoongArch ELF binary
    binary, _ := os.ReadFile("program.elf")

    // Create machine with default options
    machine, err := libloong.NewMachine(binary, nil)
    if err != nil {
        panic(err)
    }
    defer machine.Close()

    // Setup Linux environment
    libloong.SetupLinuxSyscalls()
    machine.SetupLinux([]string{"program"}, []string{})

    // Execute until completion (unlimited instructions)
    if err := machine.Simulate(^uint64(0)); err != nil {
        panic(err)
    }

    // Get exit code from $a0 register
    exitCode := machine.ReturnValue()
    fmt.Printf("Exit code: %d\n", exitCode)
}
```

### Calling Guest Functions (VMCall)

**IMPORTANT**: You can only vmcall regular functions **after** the program has been initialized. You **cannot** vmcall `main` or `_start` directly - these are entry points that require full program initialization. Use `Simulate()` to run the full program instead.

```go
// First, run the program to completion (or until a specific point)
// This ensures all initialization is complete
if err := machine.Simulate(^uint64(0)); err != nil {
    panic(err)
}

// Now you can call individual guest functions
// Call guest function by name (e.g., a library function in your ELF)
result, err := machine.VMCallByName("my_function", ^uint64(0), 10, 20)
if err != nil {
    panic(err)
}
fmt.Printf("my_function(10, 20) = %d\n", result)

// Call by address
addr := machine.AddressOf("another_function")
result, err = machine.VMCall(addr, 1_000_000, 42, 100)
```

**Common VMCall Pattern**: Load program → Initialize with `Simulate()` → Call specific functions with `VMCall*()`

### Memory Operations

```go
// Allocate writable guest memory
guestAddr := machine.MmapAllocate(4096)

// Write data to guest memory
data := []byte("Hello from Go!")
machine.CopyToGuest(guestAddr, data)

// Read data from guest memory
readData, _ := machine.CopyFromGuest(guestAddr, uint64(len(data)))
fmt.Printf("Read: %s\n", string(readData))

// Read null-terminated string
str, _ := machine.ReadString(guestAddr, 256)
```

### Accelerated Heap (Arena)

```go
// Setup accelerated heap for fast guest allocations
heapSize := uint64(1024 * 1024) // 1 MB
heapBase := machine.MmapAllocate(heapSize)
machine.SetupAcceleratedHeap(heapBase, heapSize)

// Allocate on guest heap
ptr := machine.ArenaMalloc(128)
fmt.Printf("Allocated at 0x%x\n", ptr)

// Free guest memory
machine.ArenaFree(ptr)
```

### Custom Machine Options

```go
options := libloong.MachineOptions{
    MemoryMax:                600 * 1024 * 1024, // 600 MB
    StackSize:                8 * 1024 * 1024,   // 8 MB stack
    BrkSize:                  4 * 1024 * 1024,   // 4 MB heap
    VerboseLoader:            false,
    VerboseSyscalls:          false,
    UseSharedExecuteSegments: true,
}

machine, err := libloong.NewMachine(binary, &options)
```

## API Reference

### Machine Creation

- `NewMachine(binary []byte, options *MachineOptions) (*Machine, error)` - Create new emulator instance
- `DefaultOptions() MachineOptions` - Get default configuration
- `Close()` - Free machine resources (also called by finalizer)

### Setup Functions

- `SetupLinuxSyscalls()` - Install Linux syscall handlers (global, call once)
- `SetupMinimalSyscalls()` - Install minimal syscall handlers (global)
- `SetupLinux(args []string, env []string) error` - Setup Linux environment
- `SetupAcceleratedSyscalls() error` - Enable accelerated syscalls
- `SetupAcceleratedHeap(base uint64, size uint64) error` - Setup managed heap

### Execution

- `Simulate(maxInstructions uint64) error` - Run guest program
- `Stop()` - Stop execution
- `Stopped() bool` - Check if stopped
- `InstructionLimitReached() bool` - Check if limit was hit

### Function Calls (VMCall)

**⚠️ IMPORTANT**: VMCall functions should only be used to call regular guest functions **after** program initialization. Do **not** attempt to vmcall `main`, `_start`, or other entry points - use `Simulate()` for full program execution.

- `VMCall(addr uint64, maxInstructions uint64, args ...uint64) (uint64, error)` - Call by address
- `VMCallByName(name string, maxInstructions uint64, args ...uint64) (uint64, error)` - Call by name
- `VMCallFloat(addr uint64, maxInstructions uint64, args ...uint64) (float32, error)` - Get float32 return
- `VMCallDouble(addr uint64, maxInstructions uint64, args ...uint64) (float64, error)` - Get float64 return

### Symbol Lookup

- `AddressOf(name string) uint64` - Get symbol address (0 if not found)
- `HasSymbol(name string) bool` - Check if symbol exists

### Memory Access

- `ReadMemory(addr uint64, size uint64) ([]byte, error)` - Read guest memory
- `WriteMemory(addr uint64, data []byte) error` - Write guest memory
- `ReadString(addr uint64, maxLen uint64) (string, error)` - Read null-terminated string
- `CopyToGuest(dest uint64, src []byte) error` - Copy to guest
- `CopyFromGuest(src uint64, size uint64) ([]byte, error)` - Copy from guest
- `MmapAllocate(size uint64) uint64` - Allocate writable guest memory

### Heap Allocation (Arena)

- `ArenaMalloc(size uint64) uint64` - Allocate from guest heap (returns 0 on failure)
- `ArenaFree(ptr uint64) bool` - Free guest heap memory
- `HasArena() bool` - Check if arena is setup

### Register Access

- `GetRegister(regNum uint) uint64` / `SetRegister(regNum uint, value uint64)` - Integer registers (0-31)
- `GetPC() uint64` / `SetPC(pc uint64)` - Program counter
- `GetFloatRegister(regNum uint) float32` / `SetFloatRegister(regNum uint, value float32)` - FP32 registers
- `GetDoubleRegister(regNum uint) float64` / `SetDoubleRegister(regNum uint, value float64)` - FP64 registers

### Instruction Counting

- `InstructionCounter() uint64` - Get instruction count
- `SetInstructionCounter(val uint64)` - Set counter
- `IncrementCounter(val uint64)` - Increment counter
- `MaxInstructions() uint64` / `SetMaxInstructions(val uint64)` - Instruction limit

### Return Values

- `ReturnValue() uint64` - Get integer return value from $a0

### User Data

- `SetUserdata(data unsafe.Pointer)` / `GetUserdata() unsafe.Pointer` - Store arbitrary data

## Examples

See the [examples](examples/) directory for complete working examples:

- **[hello_world.go](examples/hello_world.go)** - Load and execute a program, capture output
- **[vmcall.go](examples/vmcall.go)** - Call guest functions with arguments
- **[memory_ops.go](examples/memory_ops.go)** - Memory operations and arena allocation

### Running Examples

```bash
# Build libloong first
mkdir -p build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja loong
cd ..

# Build C wrapper
cd go
g++ -std=c++20 -c -O3 -fPIC \
    -I../rust/wrapper -I../lib \
    ../rust/wrapper/libloong_wrapper.cpp \
    -o libloong_wrapper.o
ar rcs libloong_wrapper.a libloong_wrapper.o

# Run examples
go run examples/hello_world.go ../tests/programs/hello_world.elf
go run examples/vmcall.go ../tests/programs/cxx_test.elf test_addition
go run examples/memory_ops.go ../tests/programs/cxx_test.elf
```

## Building LoongArch Guest Programs

Guest programs must be 64-bit static LoongArch ELFs with a specific load address:

```bash
loongarch64-linux-gnu-gcc -static -O2 -Wl,-Ttext-segment=0x200000 \
    -o program.elf program.c
```

The `-Wl,-Ttext-segment=0x200000` flag is **required** because memory starts at zero.

## Error Handling

All fallible operations return Go `error` values. The `Error` type provides detailed context:

```go
type Error struct {
    Code      ErrorCode      // High-level error code
    Exception ExceptionType  // Detailed exception type
    Data      uint64         // Context data (address, PC, etc.)
    Message   string         // Human-readable message
}
```

Error codes:
- `ErrorOK` - Success
- `ErrorInvalidELF` - Invalid binary format
- `ErrorExecution` - Guest execution error
- `ErrorTimeout` - Instruction limit exceeded
- `ErrorInvalidAddress` - Invalid memory address
- `ErrorSymbolNotFound` - Symbol not found
- `ErrorOutOfMemory` - Out of memory

## Performance Tips

1. **Unlimited Instructions**: Use `^uint64(0)` for `Simulate()` to enable faster dispatch without counting
2. **Binary Translation**: Built with `LA_BINARY_TRANSLATION` by default for JIT compilation
3. **Shared Execute Segments**: Set `UseSharedExecuteSegments: true` (default) for better memory usage
4. **Arena Heap**: Use arena allocation for frequently allocated guest memory

## Architecture Notes

- **Only 64-bit LoongArch (LA64)** is supported (32-bit LA32 removed)
- Uses **flat memory arena** instead of virtual paging
- **Threaded bytecode dispatch** with computed goto (GCC/Clang)
- **Block-based execution** for performance
- Based on the **libriscv** architecture

## License

This project is licensed under the same terms as the parent libloong project.

## Contributing

Contributions are welcome! Please ensure:
- Code is formatted with `gofmt`
- `go vet` passes
- Examples compile and run correctly
- CI tests pass

## Links

- [Main libloong repository](https://github.com/gonz/libloong)
- [Rust bindings](../rust/)
- [C++ library documentation](../README.md)
