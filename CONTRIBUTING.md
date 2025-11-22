# Contributing to libloong

Thank you for your interest in contributing to libloong! This document provides guidelines for contributing to the project.

## Code of Conduct

Be respectful, professional, and constructive in all interactions.

## Getting Started

1. Fork the repository
2. Clone your fork:
   ```bash
   git clone https://github.com/your-username/libloong.git
   cd libloong
   ```
3. Create a branch for your changes:
   ```bash
   git checkout -b feature/your-feature-name
   ```

## Development Setup

### Prerequisites

- CMake 3.5+
- C++20 compatible compiler (GCC 10+, Clang 12+)
- Git

### Building

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

### Running Tests

```bash
cd build
ctest --output-on-failure
```

## Making Changes

### Code Style

Follow these guidelines:

- Use 4 spaces for indentation (no tabs)
- Maximum line length: 100 characters
- Use `snake_case` for functions and variables
- Use `PascalCase` for types and classes
- Use `UPPER_CASE` for constants and macros

Example:
```cpp
namespace loongarch {
    class MyClass {
    public:
        void my_method(int parameter_name);
    private:
        int m_member_variable;
    };
}
```

### Naming Conventions

- Private members: `m_` prefix (e.g., `m_counter`)
- Constants: ALL_CAPS with underscores (e.g., `MAX_SIZE`)
- Template parameters: Single capital letter or PascalCase (e.g., `W`, `AddressType`)

### File Organization

- Header files: `.hpp` extension
- Implementation files: `.cpp` extension
- Headers in `lib/libloong/`
- Implementations in `lib/libloong/`

### Documentation

Document all public APIs with comments:

```cpp
/// @brief Simulates the machine for a specified number of instructions
/// @param max_instructions Maximum number of instructions to execute
/// @param counter Starting instruction counter value
/// @return Returns true if stopped normally, false if timeout
template <bool Throw = true>
bool simulate(uint64_t max_instructions = UINT64_MAX, uint64_t counter = 0);
```

## Types of Contributions

### Bug Fixes

1. Create an issue describing the bug
2. Reference the issue in your PR
3. Include a test case if possible

### New Features

1. Discuss the feature in an issue first
2. Break large features into smaller PRs
3. Update documentation
4. Add tests for new functionality

### Documentation

- Update relevant `.md` files in `docs/`
- Update code comments
- Add examples if appropriate

### Performance Improvements

- Include benchmarks showing improvement
- Ensure correctness is maintained
- Document any trade-offs

## Commit Guidelines

### Commit Messages

Follow conventional commits format:

```
type(scope): subject

body (optional)

footer (optional)
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation
- `style`: Formatting, no code change
- `refactor`: Code restructuring
- `perf`: Performance improvement
- `test`: Adding tests
- `chore`: Maintenance

Examples:
```
feat(cpu): add support for atomic instructions

Implements LL/SC and AMO instructions for LA64.

Closes #123
```

```
fix(memory): correct page alignment calculation

The previous calculation could cause misaligned memory access
for certain page sizes.
```

### Commit Best Practices

- Keep commits atomic and focused
- Write descriptive commit messages
- Reference issues when applicable
- Sign your commits if possible

## Pull Request Process

### Before Submitting

1. Ensure code compiles without warnings
2. Run all tests and ensure they pass
3. Update documentation if needed
4. Add tests for new functionality
5. Rebase on latest main branch

### PR Description

Include:
- What changes were made
- Why the changes were necessary
- How to test the changes
- Related issues

Example:
```markdown
## Description
Adds support for LoongArch atomic instructions (LL/SC, AMO).

## Motivation
Needed for multi-threaded guest programs.

## Testing
- Added unit tests in `tests/test_atomics.cpp`
- Tested with atomic counter benchmark

## Related Issues
Closes #123
```

### Review Process

1. Maintainer will review your PR
2. Address any feedback
3. Once approved, maintainer will merge

## Testing

### Writing Tests

Add tests in `tests/` directory:

```cpp
#include <catch2/catch.hpp>
#include <libloong/machine.hpp>

TEST_CASE("CPU can execute ADD instruction", "[cpu]") {
    loongarch::Machine<loongarch::LA64> machine { /* ... */ };
    
    // Setup test
    machine.cpu.reg(1) = 10;
    machine.cpu.reg(2) = 20;
    
    // Execute ADD instruction
    // ...
    
    // Verify result
    REQUIRE(machine.cpu.reg(3) == 30);
}
```

### Running Specific Tests

```bash
cd build
./tests/test_suite --test-case="CPU can execute ADD instruction"
```

## Project Structure

```
libloong/
├── lib/                      # Core library
│   ├── libloong/            # Headers and implementation
│   │   ├── cpu.hpp/.cpp
│   │   ├── machine.hpp/.cpp
│   │   ├── memory.hpp/.cpp
│   │   ├── linux/           # Linux syscalls
│   │   └── posix/           # POSIX support
│   └── CMakeLists.txt
├── emulator/                # Standalone emulator
│   └── src/main.cpp
├── examples/                # Example programs
├── docs/                    # Documentation
├── tests/                   # Unit tests
└── README.md
```

## Areas Needing Help

Current priorities:

1. **Instruction Implementation**: Many LoongArch instructions not yet implemented
2. **Floating-Point Support**: Complete FP instruction set
3. **Vector Extensions**: LSX/LASX support
4. **Binary Translation**: Performance optimization
5. **Test Coverage**: More comprehensive tests
6. **Documentation**: Examples and tutorials
7. **Performance**: Optimization opportunities

## Questions?

- Open an issue for questions
- Check existing issues and PRs
- Read the documentation in `docs/`

## License

By contributing, you agree that your contributions will be licensed under the MIT License.

## Attribution

libloong is based on the design of [libriscv](https://github.com/libriscv/libriscv) by fwsGonzo.

Thank you for contributing! 🎉
