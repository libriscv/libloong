# API Reference

## Core Classes

### Machine

The main emulator class that contains CPU and Memory.

#### Constructor
```cpp
Machine(std::string_view binary, const MachineOptions& options = {});
Machine(const std::vector<uint8_t>& binary, const MachineOptions& options = {});
```

#### Methods

**Execution:**
- `bool simulate(uint64_t max_instructions = UINT64_MAX, uint64_t counter = 0)` - Execute program
- `void stop()` - Stop execution
- `bool stopped() const` - Check if stopped

**System calls:**
- `void install_syscall_handler(int syscall_number, syscall_t* handler)` - Install syscall
- `address_t system_call(int syscall_number)` - Execute syscall

**Function calls:**
- `address_t vmcall(address_t func_addr, Args&&... args)` - Call guest function by address
- `address_t vmcall(const std::string& func_name, Args&&... args)` - Call guest function by name

**Memory:**
- `T read<T>(address_t addr)` - Read memory
- `void write<T>(address_t addr, T value)` - Write memory

**Counters:**
- `uint64_t instruction_counter() const` - Get instruction count
- `void set_instruction_counter(uint64_t val)` - Set instruction count
- `uint64_t max_instructions() const` - Get instruction limit
- `void set_max_instructions(uint64_t val)` - Set instruction limit

#### Public Members
- `CPU cpu` - CPU state
- `Memory memory` - Memory subsystem

---

### CPU

CPU emulation with registers and execution.

#### Methods

**Execution:**
- `bool simulate(address_t pc, uint64_t icounter, uint64_t maxcounter)` - Execute from PC
- `void simulate_inaccurate(address_t pc)` - Fast execution without instruction counting
- `void simulate_precise()` - Precise single-step execution
- `void step_one(bool use_instruction_counter = true)` - Execute one instruction

**Register access:**
- `Registers& registers()` - Get register file
- `auto& reg(uint32_t idx)` - Access general register
- `address_t pc() const` - Get program counter
- `void jump(address_t addr)` - Jump to address

**Memory:**
- `Memory& memory()` - Get memory subsystem

**Exceptions:**
- `static void trigger_exception(ExceptionType type, address_t data = 0)` - Trigger exception

---

### Memory

Memory management and paging.

#### Constructor
```cpp
Memory(Machine& machine, std::string_view binary, const MachineOptions& options);
```

#### Methods

**Memory access:**
- `T read<T>(address_t addr)` - Read typed value
- `void write<T>(address_t addr, T value)` - Write typed value
- `size_t strlen(address_t addr, size_t maxlen = 4096)` - String length
- `std::string memstring(address_t addr, size_t maxlen = 4096)` - Read string

**Memory allocation:**
- `address_t mmap_allocate(size_t size)` - Allocate memory region
- `void mmap_deallocate(address_t addr, size_t size)` - Free memory region

**Page management:**
- `Page& get_page(address_t addr)` - Get page for address
- `Page& create_page(address_t pageno)` - Create new page
- `void free_pages(address_t addr, size_t count)` - Free pages

**Information:**
- `address_t start_address() const` - Get entry point
- `address_t stack_address() const` - Get stack address
- `size_t pages_active() const` - Get active page count
- `size_t memory_usage_counter() const` - Get memory usage

---

### Registers

Register file for LoongArch CPU.

#### Members
- `address_t pc` - Program counter
- `struct { uint32_t whole; } fcsr` - Floating-point control/status

#### Methods
- `auto& get(uint32_t idx)` - Access general register
- `auto& getfl(uint32_t idx)` - Access floating-point register
- `void reset()` - Reset all registers
- `std::string to_string() const` - Debug string

---

## Register Constants

### General Purpose Registers
```cpp
REG_ZERO = 0   // Always zero
REG_RA   = 1   // Return address
REG_TP   = 2   // Thread pointer
REG_SP   = 3   // Stack pointer
REG_A0   = 4   // Argument/return 0
REG_A1   = 5   // Argument/return 1
REG_A2   = 6   // Argument 2
REG_A3   = 7   // Argument 3
REG_A4   = 8   // Argument 4
REG_A5   = 9   // Argument 5
REG_A6   = 10  // Argument 6
REG_A7   = 11  // Argument 7 (syscall number)
REG_T0-T8      // Temporaries
REG_S0-S8      // Saved registers
REG_FP   = 22  // Frame pointer
```

---

## Options

### MachineOptions
```cpp
struct MachineOptions {
    size_t memory_max = 64 * 1024 * 1024;  // Max memory
    bool verbose_loader = false;             // Verbose ELF loading
    bool ignore_text_section = false;        // Skip .text section
};
```

---

## Exceptions

### MachineException
```cpp
class MachineException : public std::runtime_error {
    ExceptionType type() const;
    uint64_t data() const;
};
```

### Exception Types
- `ILLEGAL_OPCODE` - Invalid instruction
- `ILLEGAL_OPERATION` - Invalid operation
- `PROTECTION_FAULT` - Memory protection violation
- `EXECUTION_SPACE_PROTECTION_FAULT` - Execute permission violation
- `MISALIGNED_INSTRUCTION` - Misaligned instruction fetch
- `UNIMPLEMENTED_INSTRUCTION` - Unimplemented instruction
- `MACHINE_TIMEOUT` - Instruction limit exceeded
- `OUT_OF_MEMORY` - Memory allocation failed
- `INVALID_PROGRAM` - Invalid ELF file
- `FEATURE_DISABLED` - Feature not compiled in

---

## Syscall Handler Type

```cpp
using syscall_t = void(Machine&);
```
