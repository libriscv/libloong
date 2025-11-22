# Integration Guide

## Adding libloong to your project

### Using CMake subdirectory

Add libloong as a subdirectory in your CMakeLists.txt:

```cmake
add_subdirectory(libloong/lib)
target_link_libraries(your_target loongarch)
```

### Using CMake FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(libloong
  GIT_REPOSITORY https://github.com/your-org/libloong
  GIT_TAG        main
)

FetchContent_MakeAvailable(libloong)
target_link_libraries(your_target loongarch)
```

### Using pkg-config

If you have installed libloong system-wide:

```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(libloong REQUIRED IMPORTED_TARGET libloong)

add_executable(your_program main.cpp)
target_link_libraries(your_program PkgConfig::libloong)
```

## Basic Usage

### Loading and running a program

```cpp
#include <libloong/machine.hpp>
#include <libloong/linux/syscalls.hpp>
#include <fstream>
#include <vector>

// Load ELF binary
std::ifstream file("program.elf", std::ios::binary);
std::vector<uint8_t> binary((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

// Create machine (64-bit)
loongarch::Machine<loongarch::LA64> machine { binary, {
    .memory_max = 64 * 1024 * 1024
}};

// Setup Linux syscalls
loongarch::setup_linux_syscalls(machine);

// Setup program arguments
machine.setup_linux({"program", "arg1", "arg2"}, {});

// Run
machine.simulate();
```

### Custom syscall handlers

```cpp
// Define custom syscall handler
static uint64_t my_syscall(loongarch::Machine<loongarch::LA64>& machine) {
    uint64_t arg = machine.cpu.reg(loongarch::REG_A0);
    // Handle syscall
    return 0; // Return value
}

// Install handler
machine.install_syscall_handler(1000, my_syscall);
```

### Calling guest functions

```cpp
// Call a function by address
uint64_t result = machine.vmcall(0x10000, arg1, arg2, arg3);

// Call a function by name (requires symbols)
uint64_t result = machine.vmcall("function_name", arg1, arg2);
```

## Configuration Options

### CMake Options

- `LA_32` - Enable LA32 (32-bit) support (default: ON)
- `LA_64` - Enable LA64 (64-bit) support (default: ON)
- `LA_DEBUG` - Enable debug output (default: OFF)
- `LA_BINARY_TRANSLATION` - Enable binary translation (default: OFF)
- `LA_THREADED` - Enable threading support (default: ON)
- `LA_MEMORY_TRAPS` - Enable memory access traps (default: ON)

Example:

```bash
cmake -DLA_BINARY_TRANSLATION=ON -DLA_DEBUG=ON ..
```

## Error Handling

```cpp
try {
    machine.simulate();
} catch (const loongarch::MachineException& e) {
    std::cerr << "Machine exception: " << e.what() << "\n";
    std::cerr << "Type: " << e.type() << "\n";
    std::cerr << "Data: 0x" << std::hex << e.data() << "\n";
}
```

## Memory Access

### Reading memory

```cpp
uint32_t value = machine.read<uint32_t>(address);
std::string str = machine.memory.memstring(address);
```

### Writing memory

```cpp
machine.write<uint32_t>(address, value);
```

## Advanced Features

### Serialization

```cpp
// Save state
std::vector<uint8_t> state;
machine.serialize_to(state);

// Restore state
machine.deserialize_from(state);
```

### Instruction counting

```cpp
// Get executed instruction count
uint64_t count = machine.instruction_counter();

// Set maximum instructions
machine.set_max_instructions(1000000);
```
