// Example that loads and executes a "Hello World" LoongArch program
package main

import (
	"fmt"
	"os"
	"time"

	libloong "github.com/gonz/libloong/go"
)

func main() {
	// Get ELF path from command line
	if len(os.Args) < 2 {
		fmt.Fprintf(os.Stderr, "Usage: %s <loongarch_elf_file>\n", os.Args[0])
		fmt.Fprintln(os.Stderr)
		fmt.Fprintln(os.Stderr, "Example:")
		fmt.Fprintf(os.Stderr, "  %s ../tests/programs/hello_world.elf\n", os.Args[0])
		os.Exit(1)
	}

	elfPath := os.Args[1]

	// Read the ELF binary
	binary, err := os.ReadFile(elfPath)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to read ELF file '%s': %v\n", elfPath, err)
		os.Exit(1)
	}

	fmt.Printf("Loading LoongArch ELF: %s\n", elfPath)
	fmt.Printf("Binary size: %d bytes\n", len(binary))

	// Create machine with custom options
	options := libloong.DefaultOptions()
	options.MemoryMax = 600 * 1024 * 1024

	machine, err := libloong.NewMachine(binary, &options)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to create machine: %v\n", err)
		os.Exit(1)
	}
	defer machine.Close()

	// Setup Linux environment
	libloong.SetupLinuxSyscalls()
	if err := machine.SetupLinux([]string{"program"}, []string{}); err != nil {
		fmt.Fprintf(os.Stderr, "Failed to setup Linux: %v\n", err)
		os.Exit(1)
	}

	// Execute the program (unlimited instructions)
	startTime := time.Now()
	const maxInstructions = ^uint64(0) // unlimited

	err = machine.Simulate(maxInstructions)
	elapsed := time.Since(startTime)

	if err != nil {
		fmt.Fprintf(os.Stderr, "\nExecution failed: %v\n", err)
		fmt.Fprintf(os.Stderr, "Time: %.3f seconds\n", elapsed.Seconds())

		if maxInstructions != ^uint64(0) {
			fmt.Fprintf(os.Stderr, "Instructions executed: %d\n", machine.InstructionCounter())
		}
		fmt.Fprintf(os.Stderr, "PC: 0x%016x\n", machine.GetPC())
		os.Exit(1)
	}

	exitCode := int(machine.ReturnValue()) // $a0 contains exit code

	fmt.Printf("Exit code: %d  Time: %.3f seconds\n", exitCode, elapsed.Seconds())

	// Only show instruction count if we used a limited instruction count
	// (unlimited uses faster inaccurate dispatch with no counting)
	if maxInstructions != ^uint64(0) {
		instrCount := machine.InstructionCounter()
		mips := float64(instrCount) / (elapsed.Seconds() * 1_000_000.0)
		fmt.Printf("Instructions: %d (%.2f MI/s)\n", instrCount, mips)
	}

	os.Exit(exitCode)
}
