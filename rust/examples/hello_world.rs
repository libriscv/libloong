//! Simple example that loads and executes a "Hello World" LoongArch program

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
        eprintln!("  {} ../tests/programs/hello_world.elf", args[0]);
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
    println!("Binary size: {} bytes", binary.len());

    // Create machine with default options
    let mut machine = match Machine::new(&binary, MachineOptions::default()) {
        Ok(m) => m,
        Err(e) => {
            eprintln!("Failed to create machine: {}", e);
            process::exit(1);
        }
    };

    // Setup stdout callback to capture output
    libloong::set_stdout_callback(Some(|data| {
        print!("{}", String::from_utf8_lossy(data));
    }));

    // Setup Linux environment
    Machine::setup_linux_syscalls();
    if let Err(e) = machine.setup_linux(&["program"], &[]) {
        eprintln!("Failed to setup Linux: {}", e);
        process::exit(1);
    }

    println!("Starting execution...\n");

    // Execute the program (unlimited instructions)
    match machine.simulate(u64::MAX) {
        Ok(_) => {
            println!("\n\nExecution completed successfully!");
            println!("Instructions executed: {}", machine.instruction_counter());
        }
        Err(e) => {
            eprintln!("\nExecution failed: {}", e);
            eprintln!("Instructions executed: {}", machine.instruction_counter());
            eprintln!("PC: 0x{:016x}", machine.get_pc());
            process::exit(1);
        }
    }
}
