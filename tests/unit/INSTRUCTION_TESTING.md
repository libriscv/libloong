# Instruction Testing Framework

## Overview

The instruction testing framework provides a way to test individual LoongArch instructions in isolation, without the complexity of a full program execution. This is particularly useful for:

1. **Verifying instruction implementations** - Test that instructions work correctly by only executing those specific instructions
2. **Debugging bytecode issues** - Determine whether instructions are being decoded correctly into bytecodes or falling back to slow-path
3. **Testing conditional instructions** - Verify fcmp.cond, vfcmp.cond, and xvfcmp.cond type instructions that set condition flags
4. **Testing vector operations** - Verify LASX/LSX vector instructions work correctly

## Files

- `instruction_tester.hpp` - Core testing framework that wraps the Machine/CPU API
- `test_instructions.cpp` - Example test cases using the framework

## Using the InstructionTester

### Basic Setup

```cpp
#include "instruction_tester.hpp"
using namespace loongarch::test;

TEST_CASE("My instruction test") {
    InstructionTester tester;

    // Your test code here
}
```

### Executing Single Instructions

To test a single instruction:

```cpp
InstructionTester tester;

// Set up register state
tester.set_reg(REG_A0, 42);

// Execute one instruction (encoded as uint32_t)
uint32_t instr = 0x02802004;  // li.w $a0, 8
auto result = tester.execute_one(instr, 0x10000);  // PC = 0x10000

// Verify execution succeeded
REQUIRE(result.success);
REQUIRE(result.error.empty());
REQUIRE(result.instructions_executed == 1);

// Check register values after execution
REQUIRE(tester.get_reg(REG_A0) == 8);
```

### Executing Instruction Sequences

To test a sequence of instructions:

```cpp
InstructionTester tester;

std::vector<uint32_t> instructions = {
    0x02802004,  // li.w $a0, 8
    0x02804005,  // li.w $a1, 16
    0x00108084,  // add.w $a0, $a0, $a1
};

auto result = tester.execute_sequence(instructions, 0x10000, true);

REQUIRE(result.success);
REQUIRE(result.instructions_executed == 3);
REQUIRE(tester.get_reg(REG_A0) == 24);  // 8 + 16
```

### Working with Guest Memory

Allocate aligned memory in the guest address space:

```cpp
InstructionTester tester;

// Allocate 4KB of 32-byte aligned memory
auto guest_addr = tester.allocate_guest_memory(4096, 32);
REQUIRE(guest_addr != 0);
REQUIRE((guest_addr % 32) == 0);

// Write data to guest memory
tester.write_array<double>(guest_addr, {1.0, 2.0, 3.0, 4.0});

// Read data back from guest memory
auto data = tester.read_array<double>(guest_addr, 4);
REQUIRE(data[0] == 1.0);
```

### Testing Vector Instructions

Example testing LASX vector load/store:

```cpp
InstructionTester tester;

// Allocate aligned memory
auto guest_addr = tester.allocate_guest_memory(64, 32);

// Write test data
std::vector<double> test_data = {1.5, 2.5, 3.5, 4.5};
tester.write_array<double>(guest_addr, test_data);

// Set base register
tester.set_reg(12, guest_addr);  // $t0 = guest_addr

// Execute xvld $xr1, $t0, 0
uint32_t instr = 0x2c800181;
auto result = tester.execute_one(instr);

REQUIRE(result.success);

// Verify vector register contains loaded data
auto loaded = tester.get_xvreg<double>(1);
REQUIRE_THAT(loaded[0], Catch::Matchers::WithinRel(1.5, 0.0001));
REQUIRE_THAT(loaded[1], Catch::Matchers::WithinRel(2.5, 0.0001));
```

### Testing Floating-Point Comparisons

Example testing fcmp.cond instructions:

```cpp
InstructionTester tester;

// Set up FP registers
tester.set_freg(1, 3.14159);
tester.set_freg(2, 3.14159);

// Execute fcmp.ceq.d $fcc0, $f1, $f2
uint32_t instr = 0x0c100421;
auto result = tester.execute_one(instr);

REQUIRE(result.success);

// Verify condition code was set (equal)
REQUIRE(tester.get_fcc(0) == 1);
```

## API Reference

### Memory Operations

- `allocate_guest_memory(size, alignment)` - Allocate aligned guest memory, returns guest address
- `read<T>(addr)` - Read value of type T from guest memory
- `write<T>(addr, value)` - Write value to guest memory
- `read_array<T>(addr, count)` - Read array of values
- `write_array<T>(addr, values)` - Write array of values

### Register Access

**General Purpose Registers:**
- `get_reg(reg)` - Get GPR value
- `set_reg(reg, value)` - Set GPR value

**Floating-Point Registers:**
- `get_freg(reg)` - Get FP register as double
- `set_freg(reg, value)` - Set FP register (double)
- `get_freg_f(reg)` - Get FP register as float
- `set_freg_f(reg, value)` - Set FP register (float)

**Vector Registers (LSX - 128-bit):**
- `get_vreg<T>(reg)` - Get LSX vector register as std::vector<T>
- `set_vreg<T>(reg, values)` - Set LSX vector register

**Extended Vector Registers (LASX - 256-bit):**
- `get_xvreg<T>(reg)` - Get LASX vector register as std::vector<T>
- `set_xvreg<T>(reg, values)` - Set LASX vector register

**Condition Codes:**
- `get_fcc(index)` - Get floating-point condition code (0-7)
- `set_fcc(index, value)` - Set floating-point condition code

### Execution Methods

- `execute_one(instruction, pc=0x10000)` - Execute single instruction
  - Returns `SingleInstructionResult` with success status, PC values, error message

- `execute_sequence(instructions, pc=0x10000, trace_pc=false)` - Execute sequence of instructions
  - Returns `SequenceResult` with success status, PC trace (if enabled), error message

### Utility Methods

- `reset()` - Reset machine state (registers, PC, instruction counter)
- `machine()` - Get direct access to underlying Machine<LA64>
- `dump_registers()` - Get string with all GPR values (for debugging)
- `dump_fp_registers()` - Get string with all FP register values
- `dump_xvreg_d(reg)` - Get string showing LASX register as doubles

## Real-World Example

Here's a complete example testing the LASX vector add sequence from actual code:

```cpp
TEST_CASE("LASX vector load/add/store sequence") {
    InstructionTester tester;

    // Allocate aligned memory for 4 vectors
    auto guest_addr = tester.allocate_guest_memory(4096, 32);

    // Initialize test data
    tester.write_array<double>(guest_addr + 0, {1.0, 2.0, 3.0, 4.0});
    tester.write_array<double>(guest_addr + 32, {5.0, 6.0, 7.0, 8.0});

    // Set base register
    tester.set_reg(12, guest_addr);  // $t0 = base address

    // Test sequence: load, double, store
    std::vector<uint32_t> instructions = {
        0x2c800182,  // xvld    $xr2, $t0, 0      (load vector)
        0x75310842,  // xvfadd.d $xr2, $xr2, $xr2 (double it)
        0x2cc00182,  // xvst    $xr2, $t0, 0      (store back)
    };

    auto result = tester.execute_sequence(instructions);

    REQUIRE(result.success);
    REQUIRE(result.instructions_executed == 3);

    // Verify results: [1,2,3,4] doubled = [2,4,6,8]
    auto result_vec = tester.read_array<double>(guest_addr, 4);
    REQUIRE_THAT(result_vec[0], Catch::Matchers::WithinRel(2.0, 0.0001));
    REQUIRE_THAT(result_vec[1], Catch::Matchers::WithinRel(4.0, 0.0001));
    REQUIRE_THAT(result_vec[2], Catch::Matchers::WithinRel(6.0, 0.0001));
    REQUIRE_THAT(result_vec[3], Catch::Matchers::WithinRel(8.0, 0.0001));
}
```

## Current Limitations

The tests currently show that:

1. **Instructions execute successfully** - No crashes or exceptions
2. **Instruction counting works** - Correct number of instructions executed
3. **Vector operations may need debugging** - Some vector instructions return zeros

This indicates the framework is working, but some instruction implementations may need verification. The framework is perfect for debugging these issues by:

- Testing instructions in isolation
- Checking if they're decoded to bytecodes vs slow-path (use --stats in CLI)
- Verifying register state before/after
- Comparing against objdump output (use debug_test -o)

## Integration with Other Tools

This framework complements the existing debugging tools:

- **debug_test** - Full-featured CLI debugger with objdump comparison (`./tests/debug_test -o`)
- **CLI emulator** - Fast execution with statistics (`--stats` flag shows bytecode usage)
- **Unit tests** - Automated verification of instruction correctness (this framework)

Use this framework when you need programmatic verification of instruction behavior, especially when testing edge cases or verifying fixes.
