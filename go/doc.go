// Package libloong provides Go bindings for the libloong LoongArch emulator.
//
// # Overview
//
// libloong is a high-performance 64-bit LoongArch emulator with a flat memory arena,
// designed for fast execution of LoongArch ELF binaries. It supports:
//   - Fast bytecode dispatch with optional binary translation
//   - Direct function calls (vmcall) to guest code
//   - Full Linux syscall emulation
//   - Memory operations and managed guest heap
//
// # Installation
//
// Before using this package, you need CMake and a C++20 compiler:
//
//	// Install dependencies on Ubuntu/Debian:
//	sudo apt-get install cmake g++-14
//
//	// Install the package:
//	go get github.com/gonz/libloong/go
//
//	// Build the native libraries (run once):
//	cd $GOPATH/pkg/mod/github.com/gonz/libloong@<version>/go
//	go generate
//
// # Quick Start
//
//	package main
//
//	import (
//	    "fmt"
//	    "os"
//	    "github.com/gonz/libloong/go"
//	)
//
//	func main() {
//	    // Read LoongArch ELF binary
//	    binary, _ := os.ReadFile("program.elf")
//
//	    // Create machine
//	    machine, err := libloong.NewMachine(binary, nil)
//	    if err != nil {
//	        panic(err)
//	    }
//	    defer machine.Close()
//
//	    // Setup and run
//	    libloong.SetupLinuxSyscalls()
//	    machine.SetupLinux([]string{"program"}, []string{})
//	    machine.Simulate(^uint64(0))
//
//	    fmt.Printf("Exit code: %d\n", machine.ReturnValue())
//	}
//
// # Building Guest Programs
//
// Guest programs must be 64-bit static LoongArch ELFs with a specific load address:
//
//	loongarch64-linux-gnu-gcc -static -O2 -Wl,-Ttext-segment=0x200000 \
//	    -o program.elf program.c
//
// The -Wl,-Ttext-segment=0x200000 flag is required because memory starts at zero.
//
// # Important Notes
//
// VMCall Restrictions: You can only vmcall regular functions AFTER program
// initialization. You cannot vmcall main or _start directly - use Simulate()
// to run the full program instead.
//
//go:generate go run build_libloong.go
package libloong
