# Rust Bindings for libloong

Safe Rust bindings for the [libloong](https://github.com/fwsGonzo/libloong) high-performance LoongArch 64-bit emulator.

## Features

- **Safe Rust API** - Idiomatic Rust wrappers around the C++ core
- **Zero-cost abstractions** - Minimal overhead over direct C++ usage
- **Complete coverage** - Full access to machine state, memory, and execution control
- **vmcall support** - Call guest functions from host code
- **Memory management** - Safe read/write to guest memory
- **Instruction counting** - Built-in performance tracking
- **Symbol lookup** - Find and call functions by name

## Quick Start

Add to your `Cargo.toml`:

```toml
[dependencies]
libloong = { path = "../rust" }
```

### Basic Example

```rust
use libloong::{Machine, MachineOptions};
use std::fs;

// Load a LoongArch ELF binary
let binary = fs::read("program.elf")?;

// Create machine with default options
let mut machine = Machine::new(&binary, MachineOptions::default())?;

// Setup Linux environment
Machine::setup_linux_syscalls();
machine.setup_linux(&["program"], &[])?;

// Execute the program
machine.simulate(u64::MAX)?;

println!("Instructions executed: {}", machine.instruction_counter());
```

### Calling Guest Functions (vmcall)

```rust
// Call a guest function by name with arguments
let result = machine.vmcall_by_name("factorial", &[5])?;
println!("factorial(5) = {}", result);

// Or by address
let addr = machine.address_of("factorial");
let result = machine.vmcall(addr, &[10])?;
println!("factorial(10) = {}", result);
```

### Memory Access

```rust
// Read memory
let mut buffer = vec![0u8; 256];
machine.read_memory(0x1000, &mut buffer)?;

// Write memory
let data = b"Hello, guest!";
machine.write_memory(0x2000, data)?;

// Read null-terminated strings
let string = machine.read_string(0x3000, 1024)?;
println!("Guest string: {}", string);
```

### Capturing Output

```rust
// Set a callback for guest stdout
libloong::set_stdout_callback(Some(|data| {
    print!("{}", String::from_utf8_lossy(data));
}));
```

## Building

You'll need:

1. **LoongArch cross-compiler**: `loongarch64-linux-gnu-g++` (for building test programs)
2. **CMake 3.14+** (for building libloong)
3. **Rust 1.70+**

### Build Steps

```bash
# 1. Build libloong C++ library first
cd /path/to/libloong
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# 2. Build Rust bindings
cd ../rust
cargo build --release

# 3. Run examples
cargo run --example hello_world -- ../tests/programs/hello_world.elf
cargo run --example vmcall -- ../tests/programs/cxx_test.elf test_addition
```

## Examples

### hello_world.rs

Demonstrates basic execution of a LoongArch program:

```bash
cargo run --example hello_world -- path/to/program.elf
```

### vmcall.rs

Demonstrates calling guest functions from the host:

```bash
cargo run --example vmcall -- path/to/program.elf function_name [args...]
```

## API Reference

### Machine

The main emulator struct representing a complete LoongArch machine.

```rust
// Create a new machine
pub fn new(binary: &[u8], options: MachineOptions) -> Result<Self, Error>

// Setup Linux environment
pub fn setup_linux(&mut self, args: &[&str], env: &[&str]) -> Result<(), Error>

// Execute instructions
pub fn simulate(&mut self, max_instructions: u64) -> Result<(), Error>

// Call guest functions
pub fn vmcall(&mut self, func_addr: u64, args: &[u64]) -> Result<u64, Error>
pub fn vmcall_by_name(&mut self, func_name: &str, args: &[u64]) -> Result<u64, Error>

// Memory access
pub fn read_memory(&self, addr: u64, buffer: &mut [u8]) -> Result<(), Error>
pub fn write_memory(&mut self, addr: u64, data: &[u8]) -> Result<(), Error>
pub fn read_string(&self, addr: u64, max_len: usize) -> Result<String, Error>

// Symbol lookup
pub fn address_of(&self, name: &str) -> u64
pub fn has_symbol(&self, name: &str) -> bool

// Execution control
pub fn stop(&mut self)
pub fn stopped(&self) -> bool
pub fn instruction_counter(&self) -> u64
pub fn set_max_instructions(&mut self, value: u64)

// CPU register access
pub fn get_register(&self, reg_num: u32) -> u64
pub fn set_register(&mut self, reg_num: u32, value: u64)
pub fn get_pc(&self) -> u64
pub fn set_pc(&mut self, pc: u64)
```

### MachineOptions

Configuration options for the machine:

```rust
pub struct MachineOptions {
    pub memory_max: usize,           // Default: 256 MB
    pub stack_size: usize,           // Default: 2 MB
    pub brk_size: usize,             // Default: 1 MB
    pub verbose_loader: bool,        // Default: false
    pub verbose_syscalls: bool,      // Default: false
    pub use_shared_execute_segments: bool,  // Default: true
}
```

### Error

Error types returned by libloong operations:

```rust
pub enum Error {
    InvalidElf,          // Invalid ELF binary
    Execution,           // Execution error
    Timeout,             // Instruction limit exceeded
    InvalidAddress,      // Invalid memory address
    SymbolNotFound,      // Symbol not found
    OutOfMemory,         // Out of memory
    Unknown,             // Unknown error
}
```

## Architecture

The Rust bindings consist of three layers:

1. **C++ Wrapper Layer** (`wrapper/libloong_wrapper.{cpp,h}`)
   - C-compatible FFI interface
   - Exception handling (converts C++ exceptions to error codes)
   - Opaque handle pattern for safety

2. **FFI Layer** (`src/ffi.rs`)
   - Raw Rust bindings to C wrapper
   - `extern "C"` function declarations
   - Type definitions matching C layout

3. **Safe Rust API** (`src/lib.rs`)
   - Idiomatic Rust interface
   - RAII resource management
   - Result-based error handling
   - Lifetime safety

## Building LoongArch Programs

To build guest programs for the emulator:

```bash
loongarch64-linux-gnu-g++ -O2 -static \
    -Wl,-Ttext-segment=0x200000 \
    -o program.elf program.cpp
```

The `-Wl,-Ttext-segment=0x200000` flag is **required** because the emulator uses a flat memory arena starting at address 0.

## Performance

The Rust bindings add minimal overhead:

- **Zero-cost vmcall**: Direct function call through FFI
- **Efficient memory access**: Pointer-based operations with no copying
- **Inlined accessors**: Register and counter operations compile to direct memory access

## Testing

```bash
# Build and run tests
cd rust
cargo test

# Run with verbose output
cargo test -- --nocapture

# Run specific example
cargo run --example hello_world -- ../tests/programs/return_42_bare
```

## License

MIT License - see the main libloong repository for details.

## Contributing

Contributions are welcome! Please ensure:

1. Code follows Rust conventions (`cargo fmt`, `cargo clippy`)
2. All tests pass (`cargo test`)
3. New features include examples
4. Documentation is updated

## See Also

- [libloong C++ documentation](../README.md)
- [LoongArch ISA Manual](https://loongson.github.io/LoongArch-Documentation/)
- [LoongScript Framework](../examples/script/README.md) - Higher-level C++ scripting API
