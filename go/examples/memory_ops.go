// Example demonstrating memory operations and guest heap allocation
package main

import (
	"fmt"
	"os"

	libloong "github.com/gonz/libloong/go"
)

func main() {
	// Get ELF path from command line
	if len(os.Args) < 2 {
		fmt.Fprintf(os.Stderr, "Usage: %s <loongarch_elf_file>\n", os.Args[0])
		fmt.Fprintln(os.Stderr)
		fmt.Fprintln(os.Stderr, "Example:")
		fmt.Fprintf(os.Stderr, "  %s ../tests/programs/cxx_test.elf\n", os.Args[0])
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

	// Allocate writable guest memory for testing copy operations
	heapSize := uint64(1024 * 1024) // 1 MB
	heapBegin := machine.MmapAllocate(heapSize)
	_ = machine.SetupAcceleratedHeap(heapBegin, heapSize)

	fmt.Println("\n=== Allocated writable guest memory ===")
	fmt.Printf("Address: 0x%x, size: %d bytes\n", heapBegin, heapSize)

	// Demonstrate CopyToGuest
	fmt.Println("\n=== Testing CopyToGuest ===")
	testData := []byte("Hello from the host!")
	testAddr := machine.ArenaMalloc(uint64(len(testData)))
	if err := machine.CopyToGuest(testAddr, testData); err != nil {
		fmt.Fprintf(os.Stderr, "Failed to copy to guest at 0x%x: %v\n", testAddr, err)
		os.Exit(1)
	}
	fmt.Printf("✓ Copied %d bytes to guest address 0x%x\n", len(testData), testAddr)
	fmt.Printf("  Data: %q\n", string(testData))

	// Demonstrate CopyFromGuest
	fmt.Println("\n=== Testing CopyFromGuest ===")
	readBuffer, err := machine.CopyFromGuest(testAddr, uint64(len(testData)))
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to copy from guest: %v\n", err)
		os.Exit(1)
	}
	fmt.Printf("✓ Read %d bytes from guest address 0x%x\n", len(readBuffer), testAddr)
	fmt.Printf("  Data: %q\n", string(readBuffer))

	// Verify data matches
	if string(readBuffer) == string(testData) {
		fmt.Println("✓ Data verification successful!")
	} else {
		fmt.Fprintln(os.Stderr, "✗ Data verification failed!")
		os.Exit(1)
	}

	// Setup accelerated heap (arena)
	// Use MmapAllocate to get writable memory for the arena
	arenaSize := uint64(1024 * 1024) // 1 MB
	arenaBase := machine.MmapAllocate(arenaSize)

	fmt.Println("\n=== Setting up accelerated heap (Arena) ===")
	if err := machine.SetupAcceleratedHeap(arenaBase, arenaSize); err != nil {
		fmt.Fprintf(os.Stderr, "Failed to setup accelerated heap: %v\n", err)
		os.Exit(1)
	}
	fmt.Printf("Arena base: 0x%x, size: %d bytes\n", arenaBase, arenaSize)
	fmt.Printf("Has arena: %v\n", machine.HasArena())

	// Demonstrate arena malloc/free
	fmt.Println("\n=== Testing arena malloc/free ===")
	allocSize := uint64(128)
	guestPtr := machine.ArenaMalloc(allocSize)
	if guestPtr == 0 {
		fmt.Fprintf(os.Stderr, "Failed to allocate %d bytes on guest heap\n", allocSize)
		os.Exit(1)
	}
	fmt.Printf("✓ Allocated %d bytes at guest address: 0x%x\n", allocSize, guestPtr)

	// Note: Arena memory is meant for passing pointers to vmcall
	// Direct host-to-guest copying should use WriteMemory for arena addresses
	fmt.Printf("\nNote: Arena-allocated addresses (0x%x) are for vmcall arguments\n", guestPtr)
	fmt.Println("      Use WriteMemory/ReadMemory for direct access to arena memory")

	// Free the allocated memory
	fmt.Println("\n=== Freeing allocated memory ===")
	freeResult := machine.ArenaFree(guestPtr)
	if freeResult {
		fmt.Printf("✓ Successfully freed memory at 0x%x\n", guestPtr)
	} else {
		fmt.Fprintf(os.Stderr, "✗ Failed to free memory at 0x%x\n", guestPtr)
	}

	// Allocate multiple blocks
	fmt.Println("\n=== Testing multiple allocations ===")
	type allocation struct {
		ptr  uint64
		size uint64
	}
	var allocations []allocation

	for i := 0; i < 5; i++ {
		size := uint64(64 * (i + 1))
		ptr := machine.ArenaMalloc(size)
		if ptr != 0 {
			fmt.Printf("  Allocation %d: %d bytes at 0x%x\n", i, size, ptr)
			allocations = append(allocations, allocation{ptr, size})
		} else {
			fmt.Fprintf(os.Stderr, "  Failed to allocate %d bytes\n", size)
		}
	}

	// Free all allocations
	fmt.Println("\n=== Freeing all allocations ===")
	for _, alloc := range allocations {
		result := machine.ArenaFree(alloc.ptr)
		status := "✗"
		if result {
			status = "✓"
		}
		fmt.Printf("  Freed %d bytes at 0x%x: %s\n", alloc.size, alloc.ptr, status)
	}

	fmt.Println("\n=== All tests completed successfully! ===")
}
