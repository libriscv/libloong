# libloong - Rust Bindings

Rust bindings for the libloong LoongArch emulator.

## Quick Start

Add to your `Cargo.toml`:

```toml
[dependencies]
libloong = "0.1"
```

### Example: Running a LoongArch ELF

```rust
use libloong::Machine;
use std::fs;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let binary = fs::read("program.elf")?;
    let mut machine = Machine::new(&binary, Default::default())?;

    Machine::setup_linux_syscalls();
    machine.setup_linux(&["program"], &[])?;

    machine.simulate(u64::MAX)?;

    println!("Exit code: {}", machine.return_value());
    Ok(())
}
```

### Calling Guest Functions

```rust
// Call by name and get integer return value
machine.vmcall("factorial", &[5])?;
let result = machine.return_value();
println!("factorial(5) = {}", result);

// Call by address
let func_addr = machine.address_of("factorial");
machine.vmcall(func_addr, &[5])?;
let result = machine.return_value();

// For functions returning floats:
machine.vmcall("sin_approx", &[])?;
let result_f32 = machine.return_value_f32();
let result_f64 = machine.return_value_f64();
```

## Building

Requirements:
- CMake 3.15+
- C++20 compiler (GCC 14+ or Clang 18+)
- Rust 1.70+

```bash
cargo build --release
```

The build script automatically builds the C++ library using CMake.

## Examples

Run the examples (test programs are located in `../tests/programs/`):

```bash
cargo run --example hello_world -- ../tests/programs/hello_world.elf
cargo run --example vmcall -- ../tests/programs/factorial.elf factorial 5
```

## Building LoongArch Programs

```bash
loongarch64-linux-gnu-gcc -O2 -static \
    -Wl,-Ttext-segment=0x200000 \
    -o program.elf program.c
```

The `-Wl,-Ttext-segment=0x200000` flag is required for the flat memory model.
