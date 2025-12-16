# Rust Bindings Implementation Notes

This document describes the implementation details of the libloong Rust bindings.

## Architecture

The bindings use a three-layer architecture inspired by the LoongScript library:

### Layer 1: C++ Wrapper (wrapper/)

**Purpose**: Provide a C-compatible FFI boundary

**Files**:
- `libloong_wrapper.h` - C API declarations
- `libloong_wrapper.cpp` - C wrapper implementation

**Key Features**:
- Opaque `LibLoongMachine` handle type for safety
- Exception boundary - catches all C++ exceptions and converts to error codes
- C-compatible types (no C++ classes/references in the API)
- Static callback storage for stdout handling

**Exception Handling**:
```cpp
template<typename F>
LibLoongError safe_call(F&& func, LibLoongError default_error) {
    try {
        func();
        return LIBLOONG_OK;
    } catch (const MachineTimeoutException&) {
        return LIBLOONG_ERROR_TIMEOUT;
    } catch (const MachineException&) {
        return LIBLOONG_ERROR_EXECUTION;
    }
    // ... more exception types
}
```

### Layer 2: FFI Bindings (src/ffi.rs)

**Purpose**: Raw Rust declarations for the C API

**Key Features**:
- `#[repr(C)]` structs matching C layout exactly
- `extern "C"` function declarations
- Error enum with `#[repr(C)]` for ABI compatibility
- Link directives for the static libraries

**Linking**:
```rust
#[link(name = "loong_wrapper", kind = "static")]
#[link(name = "loong", kind = "static")]
#[link(name = "stdc++")]
```

### Layer 3: Safe Rust API (src/lib.rs)

**Purpose**: Idiomatic Rust interface with safety guarantees

**Key Features**:
- RAII with automatic cleanup via `Drop` trait
- Result-based error handling
- Lifetime safety (no dangling pointers)
- Type-safe enums instead of raw error codes
- Convenient methods with sensible defaults

**Example**:
```rust
pub struct Machine {
    handle: *mut ffi::LibLoongMachine,
    _marker: PhantomData<ffi::LibLoongMachine>,
}

impl Drop for Machine {
    fn drop(&mut self) {
        unsafe {
            ffi::libloong_machine_destroy(self.handle);
        }
    }
}
```

## Build Process

The `build.rs` script:

1. **Compiles C++ Wrapper**: Uses `cc` crate to compile `libloong_wrapper.cpp`
   - Enables C++20 and exceptions (needed for safe_call)
   - Links against pre-built `libloong.a`
   - Includes both `lib/` and `build/lib/` directories

2. **Links Libraries**:
   - `libloong.a` - Main emulator library
   - `libloong_wrapper.a` - Our C wrapper (built by cc crate)
   - `libstdc++` - C++ standard library (Linux) or `libc++` (macOS)

3. **Build Configuration**:
   ```rust
   build.compile("loong_wrapper");  // Produces libloong_wrapper.a
   println!("cargo:rustc-link-search=native={}", "build/lib");
   println!("cargo:rustc-link-lib=static=loong");
   ```

## API Design Decisions

### 1. Error Handling

**C++ → C → Rust conversion**:
- C++ exceptions → C error codes → Rust `Result<T, Error>`
- All fallible operations return `Result`
- No panics in normal operation

### 2. Memory Safety

**Ownership**:
- `Machine` owns its handle
- Cannot be cloned (no `Clone` trait)
- Can be moved (has `Send` trait for single-threaded transfer)
- Automatic cleanup via `Drop`

**Borrowed References**:
- Const methods take `&self`
- Mutable methods take `&mut self`
- No way to get a raw pointer except via `as_raw()` (unsafe escape hatch)

### 3. stdout Callback

**Implementation**:
- Global static callback (one per process)
- Thread-safe via `unsafe` static
- Uses libloong's new `set_print_callback()` static method
- Callback wrapper translates C string to Rust `&[u8]`

### 4. vmcall Support

**Current Limitation**:
- Direct vmcall requires C runtime initialization
- Solution: Run main() first, then call functions
- This matches the LoongScript pattern

**Future Enhancement**:
Could add a `Machine::initialize_crt()` method to set up the C runtime without running main.

## Testing Strategy

### Unit Tests
```rust
#[cfg(test)]
mod tests {
    #[test]
    fn test_machine_options_default() {
        let opts = MachineOptions::default();
        assert_eq!(opts.memory_max, 256 * 1024 * 1024);
    }
}
```

### Example Programs
1. **hello_world.rs** - Full program execution
2. **vmcall.rs** - Function calls (requires main to run first)

### CI Pipeline
See `.github/workflows/rust-bindings.yml`:
- Builds libloong C++ library
- Compiles Rust bindings
- Runs cargo fmt/clippy
- Builds test programs (return_42, hello_world, factorial)
- Validates example output

## Performance Considerations

### Zero-Cost Abstractions
- Most methods inline to direct FFI calls
- No runtime overhead for safety (borrow checker is compile-time)
- RAII cleanup is a single destructor call

### Optimization Levels
- Release builds use `-O2` for the C++ wrapper
- Rust code compiles with `--release` for production
- No LTO between C++ and Rust (not needed, minimal FFI surface)

## Common Pitfalls

### 1. Forgetting to Initialize
```rust
// WRONG: vmcall without initialization
let mut machine = Machine::new(&binary, options)?;
machine.vmcall_by_name("foo", &[])?; // Will fail!

// CORRECT: Initialize first
Machine::setup_linux_syscalls();
machine.setup_linux(&["program"], &[])?;
machine.simulate(u64::MAX)?; // Run main()
// Now C runtime is initialized
```

### 2. Lifetimes
```rust
// WRONG: Machine dropped before using its memory
let addr = {
    let machine = Machine::new(&binary, options)?;
    machine.address_of("foo") // addr is now dangling!
};

// CORRECT: Keep machine alive
let machine = Machine::new(&binary, options)?;
let addr = machine.address_of("foo");
// Use addr while machine is alive
```

### 3. Thread Safety
```rust
// Machine is Send but not Sync
// Can transfer between threads but not share
let machine = Machine::new(&binary, options)?;
std::thread::spawn(move || {
    machine.simulate(u64::MAX) // OK: moved to thread
});
```

## Future Enhancements

### Potential Features
1. **Per-machine callbacks** - Per-instance stdout/syscall hooks
2. **Async support** - Tokio integration for non-blocking execution
3. **Syscall interface** - Expose custom syscall registration
4. **Binary translation control** - Expose bintr options
5. **Signal handling** - Guest signal delivery
6. **Multicore** - Thread support (when added to libloong)

### API Stability
- Current API is 0.1.x (unstable)
- Breaking changes expected before 1.0
- Feedback welcome via GitHub issues

## Comparison with LoongScript

### Similarities
- Three-layer architecture (C++/C/High-level)
- Exception boundaries
- RAII resource management
- Comprehensive examples

### Differences
- Rust has stronger lifetime checking than C++
- No complex type translation (GuestStdString, etc.) needed
- Result<T, E> instead of try/catch
- No dynamic compilation (Rust can't invoke cross-compiler at runtime)

## References

- [libloong C++ API](../lib/libloong/machine.hpp)
- [LoongScript Implementation](../examples/script/)
- [Rust FFI Guidelines](https://doc.rust-lang.org/nomicon/ffi.html)
- [The Rust Programming Language](https://doc.rust-lang.org/book/)
