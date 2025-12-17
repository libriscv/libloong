//! Simple example that loads and executes a "Hello World" LoongArch program

use libloong::{Machine, MachineOptions};
use std::env;
use std::fs;
use std::process;
use std::time::Instant;

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

    // Create machine with custom options
    let options = MachineOptions {
        memory_max: 600 * 1024 * 1024,
        ..Default::default()
    };
    let mut machine = match Machine::new(&binary, options) {
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

    // Execute the program (unlimited instructions)
    let start_time = Instant::now();
    let max_instructions = u64::MAX;

    match machine.simulate(max_instructions) {
        Ok(_) => {
            let elapsed = start_time.elapsed();
            let exit_code = machine.return_value() as i32; // $a0 (r4) contains exit code

            println!(
                "Exit code: {}  Time: {:.3} seconds",
                exit_code,
                elapsed.as_secs_f64()
            );

            // Only show instruction count if we used a limited instruction count
            // (unlimited uses faster inaccurate dispatch with no counting)
            if max_instructions != u64::MAX {
                let instr_count = machine.instruction_counter();
                let mips = instr_count as f64 / (elapsed.as_secs_f64() * 1_000_000.0);
                println!("Instructions: {} ({:.2} MI/s)", instr_count, mips);
            }

            process::exit(exit_code);
        }
        Err(e) => {
            let elapsed = start_time.elapsed();
            eprintln!("\nExecution failed: {}", e);
            eprintln!("Time: {:.3} seconds", elapsed.as_secs_f64());

            if max_instructions != u64::MAX {
                eprintln!("Instructions executed: {}", machine.instruction_counter());
            }
            eprintln!("PC: 0x{:016x}", machine.get_pc());
            process::exit(1);
        }
    }
}
