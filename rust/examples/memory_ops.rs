//! Example demonstrating memory operations and guest heap allocation

use libloong::{Machine, MachineOptions};
use std::env;
use std::fs;
use std::process;

fn main() {
    // Get ELF path from command line
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        eprintln!("Usage: {} <loongarch_elf_file>", args[0]);
        eprintln!();
        eprintln!("Example:");
        eprintln!("  {} ../tests/programs/cxx_test.elf", args[0]);
        process::exit(1);
    }

    let elf_path = &args[1];

    // Read the ELF binary
    let binary = match fs::read(elf_path) {
        Ok(data) => data,
        Err(e) => {
            eprintln!("Failed to read ELF file '{}': {}", elf_path, e);
            process::exit(1);
        }
    };

    println!("Loading LoongArch ELF: {}", elf_path);

    // Create machine
    let mut machine = match Machine::new(&binary, MachineOptions::default()) {
        Ok(m) => m,
        Err(e) => {
            eprintln!("Failed to create machine: {}", e);
            process::exit(1);
        }
    };

    // Setup Linux environment
    Machine::setup_linux_syscalls();
    if let Err(e) = machine.setup_linux(&["program"], &[]) {
        eprintln!("Failed to setup Linux: {}", e);
        process::exit(1);
    }

    // Allocate writable guest memory for testing copy operations
    let heap_size = 1024 * 1024; // 1 MB
    let heap_begin = machine.mmap_allocate(heap_size);
    let _ = machine.setup_accelerated_heap(heap_begin, heap_size);

    println!("\n=== Allocated writable guest memory ===");
    println!("Address: 0x{:x}, size: {} bytes", heap_begin, heap_size);

    // Demonstrate copy_to_guest
    println!("\n=== Testing copy_to_guest ===");
    let test_data = b"Hello from the host!";
    let test_addr = machine.arena_malloc(test_data.len());
    if let Err(e) = machine.copy_to_guest(test_addr, test_data) {
        eprintln!("Failed to copy to guest at 0x{:x}: {}", test_addr, e);
        process::exit(1);
    }
    println!(
        "✓ Copied {} bytes to guest address 0x{:x}",
        test_data.len(),
        test_addr
    );
    println!("  Data: {:?}", std::str::from_utf8(test_data).unwrap());

    // Demonstrate copy_from_guest
    println!("\n=== Testing copy_from_guest ===");
    let mut read_buffer = vec![0u8; test_data.len()];
    if let Err(e) = machine.copy_from_guest(&mut read_buffer, test_addr) {
        eprintln!("Failed to copy from guest: {}", e);
        process::exit(1);
    }
    println!(
        "✓ Read {} bytes from guest address 0x{:x}",
        read_buffer.len(),
        test_addr
    );
    println!("  Data: {:?}", std::str::from_utf8(&read_buffer).unwrap());

    // Verify data matches
    if read_buffer == test_data {
        println!("✓ Data verification successful!");
    } else {
        eprintln!("✗ Data verification failed!");
        process::exit(1);
    }

    // Setup accelerated heap (arena)
    // Use mmap_allocate to get writable memory for the arena
    let arena_size = 1024 * 1024; // 1 MB
    let arena_base = machine.mmap_allocate(arena_size);

    println!("\n=== Setting up accelerated heap (Arena) ===");
    if let Err(e) = machine.setup_accelerated_heap(arena_base, arena_size) {
        eprintln!("Failed to setup accelerated heap: {}", e);
        process::exit(1);
    }
    println!("Arena base: 0x{:x}, size: {} bytes", arena_base, arena_size);
    println!("Has arena: {}", machine.has_arena());

    // Demonstrate arena malloc/free
    println!("\n=== Testing arena malloc/free ===");
    let alloc_size = 128;
    let guest_ptr = machine.arena_malloc(alloc_size);
    if guest_ptr == 0 {
        eprintln!("Failed to allocate {} bytes on guest heap", alloc_size);
        process::exit(1);
    }
    println!(
        "✓ Allocated {} bytes at guest address: 0x{:x}",
        alloc_size, guest_ptr
    );

    // Note: Arena memory is meant for passing pointers to vmcall
    // Direct host-to-guest copying should use write_memory for arena addresses
    println!(
        "\nNote: Arena-allocated addresses (0x{:x}) are for vmcall arguments",
        guest_ptr
    );
    println!("      Use write_memory/read_memory for direct access to arena memory");

    // Free the allocated memory
    println!("\n=== Freeing allocated memory ===");
    let free_result = machine.arena_free(guest_ptr);
    if free_result == 0 {
        println!("✓ Successfully freed memory at 0x{:x}", guest_ptr);
    } else {
        eprintln!("✗ Failed to free memory at 0x{:x}", guest_ptr);
    }

    // Allocate multiple blocks
    println!("\n=== Testing multiple allocations ===");
    let mut allocations = Vec::new();
    for i in 0..5 {
        let size = 64 * (i + 1);
        let ptr = machine.arena_malloc(size);
        if ptr != 0 {
            println!("  Allocation {}: {} bytes at 0x{:x}", i, size, ptr);
            allocations.push((ptr, size));
        } else {
            eprintln!("  Failed to allocate {} bytes", size);
        }
    }

    // Free all allocations
    println!("\n=== Freeing all allocations ===");
    for (ptr, size) in allocations {
        let result = machine.arena_free(ptr);
        println!(
            "  Freed {} bytes at 0x{:x}: {}",
            size,
            ptr,
            if result == 0 { "✓" } else { "✗" }
        );
    }

    println!("\n=== All tests completed successfully! ===");
}
