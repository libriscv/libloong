# libloong Unit Tests

This directory contains the unit testing framework for libloong, featuring a codebuilder system similar to libriscv that compiles C and C++ test programs on-the-fly.

## Features

- **CodeBuilder**: Compile C and C++ programs to LoongArch binaries at test time
- **TestMachine**: Convenient wrapper for Machine instantiation and execution
- **Comprehensive Tests**: Basic operations, C++ features, vmcall, and machine state
- **Catch2 Integration**: Modern C++ testing framework with BDD-style tests

## Requirements

- LoongArch cross-compiler: `loongarch64-linux-gnu-gcc-14` or `loongarch64-linux-gnu-gcc`
- Catch2 v3 (included as submodule in tests/Catch2)
- CMake 3.10 or later

## Building

From the project root:

```bash
mkdir build && cd build
cmake ..
make
```

## Running Tests

### Run all tests
```bash
cd build
make test
```

### Run with verbose output
```bash
make check
```

### Run unit tests directly
```bash
./tests/unit/unit_tests
```

### Run specific tests
```bash
# Run only basic tests
./tests/unit/unit_tests "[basic]"

# Run only C++ tests
./tests/unit/unit_tests "[cpp]"

# Run vmcall tests
./tests/unit/unit_tests "[vmcall]"

# Run machine state tests
./tests/unit/unit_tests "[machine]"

# List all available tests
./tests/unit/unit_tests --list-tests
```

## Test Structure

### Core Components

- **[codebuilder.hpp](codebuilder.hpp)**: On-the-fly C/C++ compilation to LoongArch
- **[test_utils.hpp](test_utils.hpp)**: TestMachine wrapper and execution helpers
- **[unit_main.cpp](unit_main.cpp)**: Catch2 main entry point

### Test Files

- **[test_basic.cpp](test_basic.cpp)**: Basic C functionality (arithmetic, control flow, functions, arrays)
- **[test_cpp.cpp](test_cpp.cpp)**: C++ features (classes, inheritance, templates, STL, exceptions)
- **[test_vmcall.cpp](test_vmcall.cpp)**: Guest function calls from host (vmcall)
- **[test_machine.cpp](test_machine.cpp)**: Machine state, memory, registers, symbols

## Writing Tests

### Basic Test Example

```cpp
#include <catch2/catch_test_macros.hpp>
#include "codebuilder.hpp"
#include "test_utils.hpp"

using namespace loongarch;
using namespace loongarch::test;

TEST_CASE("My test", "[mytag]") {
    CodeBuilder builder;

    SECTION("Simple test") {
        auto binary = builder.build(R"(
            int main() {
                return 42;
            }
        )", "my_test");

        auto result = run_binary(binary, 42);
        REQUIRE(result.success);
        REQUIRE(result.exit_code == 42);
    }
}
```

### Using TestMachine

```cpp
TEST_CASE("Machine test", "[machine]") {
    CodeBuilder builder;

    auto binary = builder.build(R"(
        int add(int a, int b) {
            return a + b;
        }

        int main() {
            return 0;
        }
    )", "machine_test");

    TestMachine machine(binary);
    machine.setup_linux();

    // Call guest function from host
    int result = machine.vmcall("add", 15, 27);
    REQUIRE(result == 42);

    // Access machine state
    auto addr = machine.address_of("add");
    REQUIRE(addr != 0);
}
```

### C++ Program Testing

```cpp
TEST_CASE("C++ test", "[cpp]") {
    CodeBuilder builder;

    auto binary = builder.build_cpp(R"(
        #include <iostream>

        class MyClass {
        public:
            int getValue() { return 42; }
        };

        int main() {
            MyClass obj;
            return obj.getValue();
        }
    )", "cpp_test");

    auto result = run_binary(binary, 42);
    REQUIRE(result.success);
}
```

## Compiler Options

The `CodeBuilder` supports various compiler options:

```cpp
CompilerOptions opts;
opts.static_linking = true;        // -static
opts.nostdlib = false;             // -nostdlib
opts.nostartfiles = false;         // -nostartfiles
opts.optimization = 2;             // -O2
opts.debug_info = true;            // -g
opts.text_segment = "0x200000";    // -Wl,-Ttext-segment=
opts.defines.push_back("MYDEFINE"); // -DMYDEFINE
opts.extra_flags.push_back("-Wall");

auto binary = builder.build(source, "name", opts);
```

## Test Utilities

### ExecutionResult

```cpp
struct ExecutionResult {
    bool success;                   // Did program complete successfully?
    int exit_code;                  // Program exit code
    uint64_t instructions_executed; // Number of instructions run
    std::string error;              // Error message if failed
    address_type<LA64> final_pc;    // Final program counter
    bool reached_main;              // Did execution reach main()?
};
```

### Helper Functions

```cpp
// Run a binary with expected exit code
auto result = run_binary(binary, expected_exit_code);

// Create simple C program template
std::string source = make_c_program("int main() { return 42; }");

// Create C++ program template
std::string source = make_cpp_program("int main() { return 42; }");

// Create bare metal program (no libc)
std::string source = make_bare_program("// bare metal code");
```

## Tags

Tests are organized with tags for easy filtering:

- `[basic]` - Basic C functionality
- `[control]` - Control flow (if, loops)
- `[functions]` - Function calls and recursion
- `[memory]` - Memory and array operations
- `[stdio]` - Standard I/O functions
- `[cpp]` - C++ features
- `[inheritance]` - C++ inheritance
- `[templates]` - C++ templates
- `[stdlib]` - C++ standard library
- `[operators]` - C++ operator overloading
- `[exceptions]` - C++ exception handling
- `[vmcall]` - Host-to-guest function calls
- `[advanced]` - Advanced vmcall scenarios
- `[machine]` - Machine state and operations
- `[registers]` - Register access
- `[performance]` - Instruction counting
- `[symbols]` - Symbol lookup
- `[state]` - Machine state queries
- `[pc]` - Program counter tests

## CI Integration

Unit tests are automatically run in GitHub Actions on every push and pull request. See [.github/workflows/tests.yml](../../.github/workflows/tests.yml).

## Troubleshooting

### Compiler Not Found

If you see "LoongArch compiler not found", install the cross-compiler:

```bash
# Debian/Ubuntu
sudo apt-get install gcc-loongarch64-linux-gnu g++-loongarch64-linux-gnu

# Or build from source
# See: https://github.com/loongson/build-tools
```

### Catch2 Not Found

If Catch2 is not found, initialize the submodule:

```bash
git submodule update --init --recursive
```

Or install Catch2 system-wide:

```bash
# Debian/Ubuntu
sudo apt-get install catch2

# Or from source
git clone https://github.com/catchorg/Catch2.git
cd Catch2
cmake -Bbuild -H. -DBUILD_TESTING=OFF
sudo cmake --build build/ --target install
```

## Architecture

The unit testing framework follows the libriscv model:

1. **Test Definition**: Tests are written using Catch2 macros
2. **Code Generation**: `CodeBuilder` compiles C/C++ source to LoongArch ELF
3. **Machine Creation**: `TestMachine` wraps `Machine<LA64>` with helpers
4. **Execution**: Programs run inside the emulated LoongArch machine
5. **Verification**: Results are checked using Catch2 assertions

This allows testing the full emulation stack including:
- Instruction execution
- System call handling
- Memory management
- Symbol resolution
- Host-guest interaction (vmcall)

## Contributing

When adding new tests:

1. Choose the appropriate test file or create a new one
2. Use descriptive test names and sections
3. Add appropriate tags for filtering
4. Keep tests focused and independent
5. Document complex test scenarios
6. Ensure tests clean up after themselves

Example:

```cpp
TEST_CASE("New feature", "[feature][category]") {
    SECTION("Specific scenario") {
        // Test code here
    }

    SECTION("Edge case") {
        // Edge case test
    }
}
```
