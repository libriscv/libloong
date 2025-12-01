# Examples

## Basic Usage Examples

### 1. Simple Hello World Emulator

```cpp
#include <libloong/machine.hpp>
#include <iostream>
#include <fstream>
#include <vector>

std::vector<uint8_t> load_file(const char* filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    file.read((char*)buffer.data(), size);
    return buffer;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <program.elf>\n";
        return 1;
    }

    try {
        auto binary = load_file(argv[1]);

        loongarch::Machine machine { binary };
        machine.setup_linux_syscalls();
        machine.setup_linux({argv[1]}, {"LC_ALL=C"});

        machine.simulate();

        return machine.cpu.reg(loongarch::REG_A0);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
```

### 2. Custom Syscall Handler

```cpp
#include <libloong/machine.hpp>
#include <iostream>

// Custom syscall: print a debug message
uint64_t debug_print_syscall(loongarch::Machine& machine) {
    auto msg_addr = machine.cpu.reg(loongarch::REG_A0);
    auto msg_len = machine.cpu.reg(loongarch::REG_A1);

    std::string message;
    for (size_t i = 0; i < msg_len; i++) {
        message += machine.memory.read<char>(msg_addr + i);
    }

    std::cout << "[DEBUG] " << message << std::endl;
    return 0;
}

int main() {
    std::vector<uint8_t> binary = /* ... */;

    loongarch::Machine machine { binary };

    // Install custom syscall at number 1000
    machine.install_syscall_handler(1000, debug_print_syscall);

    machine.simulate();
    return 0;
}
```

### 3. Call Guest Functions

```cpp
#include <libloong/machine.hpp>
#include <iostream>

int main() {
    std::vector<uint8_t> binary = /* ... */;

    loongarch::Machine machine { binary };
    loongarch::setup_linux_syscalls(machine);

    // Call a guest function by address with arguments
    uint64_t result = machine.vmcall(0x10000, 42, 100);
    std::cout << "Function returned: " << result << "\n";

    // Or by name (if symbols are available)
    result = machine.vmcall("calculate", 10, 20);
    std::cout << "Calculate returned: " << result << "\n";

    return 0;
}
```

### 4. Memory Inspection

```cpp
#include <libloong/machine.hpp>
#include <iostream>
#include <iomanip>

void dump_memory(loongarch::Machine& machine,
                 uint64_t addr, size_t len) {
    std::cout << "Memory at 0x" << std::hex << addr << ":\n";
    for (size_t i = 0; i < len; i += 16) {
        std::cout << std::hex << std::setfill('0') << std::setw(8) << (addr + i) << ": ";
        for (size_t j = 0; j < 16 && i + j < len; j++) {
            uint8_t byte = machine.memory.read<uint8_t>(addr + i + j);
            std::cout << std::setw(2) << (int)byte << " ";
        }
        std::cout << "\n";
    }
    std::cout << std::dec;
}

int main() {
    std::vector<uint8_t> binary = /* ... */;

    loongarch::Machine machine { binary };

    // Dump stack memory
    dump_memory(machine, machine.cpu.reg(loongarch::REG_SP), 256);

    return 0;
}
```

### 5. Step-by-Step Debugging

```cpp
#include <libloong/machine.hpp>
#include <libloong/debug.hpp>
#include <iostream>

int main() {
    std::vector<uint8_t> binary = /* ... */;

    loongarch::Machine machine { binary };
    loongarch::setup_linux_syscalls(machine);
    loongarch::DebugMachine debug { machine };

    // Execute 100 instructions with debug output
    for (int i = 0; i < 100; i++) {
        std::cout << "\n=== Instruction " << i << " ===\n";
        debug.print_instruction();

        machine.cpu.step_one();

        debug.print_registers();

        if (machine.stopped()) {
            std::cout << "Program exited\n";
            break;
        }
    }

    return 0;
}
```

### 6. Execution Timeout Handling

```cpp
#include <libloong/machine.hpp>
#include <iostream>

int main() {
    std::vector<uint8_t> binary = /* ... */;

    loongarch::Machine machine { binary };
    loongarch::setup_linux_syscalls(machine);
    machine.setup_linux({"program"}, {});

    try {
        // Limit to 1 million instructions
        constexpr uint64_t MAX_INSTR = 1000000;
        bool completed = machine.simulate<false>(MAX_INSTR);

        if (completed) {
            std::cout << "Program finished normally\n";
        } else {
            std::cout << "Execution timeout after "
                      << machine.instruction_counter()
                      << " instructions\n";
        }
    } catch (const loongarch::MachineException& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
```

### 7. Serialization / Checkpointing

```cpp
#include <libloong/machine.hpp>
#include <iostream>
#include <fstream>

int main() {
    std::vector<uint8_t> binary = /* ... */;

    loongarch::Machine machine { binary };
    loongarch::setup_linux_syscalls(machine);

    // Run for a while
    machine.simulate<false>(100000);

    // Save state
    std::vector<uint8_t> checkpoint;
    machine.serialize_to(checkpoint);

    std::ofstream out("checkpoint.bin", std::ios::binary);
    out.write((char*)checkpoint.data(), checkpoint.size());
    out.close();

    std::cout << "Checkpoint saved (" << checkpoint.size() << " bytes)\n";

    // Later: restore state
    std::ifstream in("checkpoint.bin", std::ios::binary);
    std::vector<uint8_t> restored((std::istreambuf_iterator<char>(in)),
                                   std::istreambuf_iterator<char>());

    machine.deserialize_from(restored);
    std::cout << "Checkpoint restored\n";

    // Continue execution
    machine.simulate();

    return 0;
}
```

## Building the Examples

```bash
# Create build directory
mkdir build && cd build

# Configure with examples
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)

# Run example
./example_program path/to/loongarch/program.elf
```

## Testing with Simple Programs

### Fibonacci Example (C code for LoongArch)

```c
// fib.c - Compile with LoongArch GCC
#include <stdio.h>

long fib(long n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int main() {
    printf("fib(10) = %ld\n", fib(10));
    return 0;
}
```

Compile:
```bash
loongarch64-linux-gnu-gcc-14 -static -O2 -o fib fib.c
```

Run in emulator:
```bash
./laemu fib
```
