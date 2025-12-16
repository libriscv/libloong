//! Example demonstrating vmcall - calling guest functions from host

use libloong::{Machine, MachineOptions};
use std::env;
use std::fs;
use std::process;

fn main() {
    // Get ELF path from command line
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        eprintln!(
            "Usage: {} <loongarch_elf_file> [function_name] [args...]",
            args[0]
        );
        eprintln!();
        eprintln!("Example:");
        eprintln!("  {} ../tests/programs/cxx_test.elf test_addition", args[0]);
        process::exit(1);
    }

    let elf_path = &args[1];
    let func_name = args.get(2).map(|s| s.as_str()).unwrap_or("main");

    // Parse remaining arguments as u64
    let func_args: Vec<u64> = args[3..].iter().filter_map(|s| s.parse().ok()).collect();

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

    // Setup stdout callback
    libloong::set_stdout_callback(Some(|data| {
        print!("{}", String::from_utf8_lossy(data));
    }));

    // Check if function exists
    if !machine.has_symbol(func_name) {
        eprintln!("Function '{}' not found in binary", func_name);
        eprintln!("\nTry running with a different function name.");
        process::exit(1);
    }

    let func_addr = machine.address_of(func_name);
    println!(
        "Calling function '{}' at address 0x{:x}",
        func_name, func_addr
    );
    println!("Arguments: {:?}", func_args);
    println!();

    // Call the guest function
    match machine.vmcall_by_name(func_name, &func_args) {
        Ok(result) => {
            println!("\nFunction returned: {} (0x{:x})", result, result);
            println!("Instructions executed: {}", machine.instruction_counter());
        }
        Err(e) => {
            eprintln!("\nVmcall failed: {}", e);
            eprintln!("Instructions executed: {}", machine.instruction_counter());
            eprintln!("PC: 0x{:016x}", machine.get_pc());
            process::exit(1);
        }
    }
}
