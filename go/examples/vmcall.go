// Example demonstrating vmcall - calling guest functions from host
package main

import (
	"fmt"
	"os"
	"strconv"

	libloong "github.com/gonz/libloong/go"
)

func main() {
	// Get ELF path from command line
	if len(os.Args) < 2 {
		fmt.Fprintf(os.Stderr, "Usage: %s <loongarch_elf_file> [function_name] [args...]\n", os.Args[0])
		fmt.Fprintln(os.Stderr)
		fmt.Fprintln(os.Stderr, "Example:")
		fmt.Fprintf(os.Stderr, "  %s ../tests/programs/cxx_test.elf test_addition\n", os.Args[0])
		os.Exit(1)
	}

	elfPath := os.Args[1]
	funcName := "test_function" // Don't use "main" or "_start" - only regular functions after init
	if len(os.Args) > 2 {
		funcName = os.Args[2]
	}

	// Parse remaining arguments as uint64
	var funcArgs []uint64
	for _, arg := range os.Args[3:] {
		if val, err := strconv.ParseUint(arg, 0, 64); err == nil {
			funcArgs = append(funcArgs, val)
		}
	}

	// Read the ELF binary
	binary, err := os.ReadFile(elfPath)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to read ELF file '%s': %v\n", elfPath, err)
		os.Exit(1)
	}

	fmt.Printf("Loading LoongArch ELF: %s\n", elfPath)

	// Create machine
	machine, err := libloong.NewMachine(binary, nil)
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

	// Check if function exists
	if !machine.HasSymbol(funcName) {
		fmt.Fprintf(os.Stderr, "Function '%s' not found in binary\n", funcName)
		fmt.Fprintln(os.Stderr, "\nNOTE: You can only vmcall regular functions, not 'main' or '_start'.")
		fmt.Fprintln(os.Stderr, "      These entry points need full initialization.")
		fmt.Fprintln(os.Stderr, "      Use Simulate() to run the full program instead.")
		os.Exit(1)
	}

	fmt.Printf("Calling function '%s'\n", funcName)
	fmt.Printf("Arguments: %v\n", funcArgs)
	fmt.Println()
	fmt.Println("NOTE: This calls the function AFTER the program has initialized.")

	// Call the guest function
	result, err := machine.VMCallByName(funcName, ^uint64(0), funcArgs...)
	if err != nil {
		fmt.Fprintf(os.Stderr, "\nVmcall failed: %v\n", err)
		fmt.Fprintf(os.Stderr, "Instructions executed: %d\n", machine.InstructionCounter())
		fmt.Fprintf(os.Stderr, "PC: 0x%016x\n", machine.GetPC())
		os.Exit(1)
	}

	fmt.Printf("\nFunction returned: %d (0x%x)\n", result, result)
	fmt.Printf("Instructions executed: %d\n", machine.InstructionCounter())
}
