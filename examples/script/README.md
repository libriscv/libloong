# LibLoong Script Framework

A high-performance C++ scripting framework built on libloong with bidirectional host-guest bindings.

## Quick Start

```cpp
#include "script.hpp"
#include "host_bindings.hpp"
using namespace loongarch::script;

Script script = Script::from_source(R"(
    extern "C" int factorial(int n) {
        if (n <= 1) return 1;
        return n * factorial(n - 1);
    }
)");

int result = script.call<int>("factorial", 5);  // Returns 120
```

## Core Features

### 1. Script Execution

Compile and run C++ code at runtime:

```cpp
Script script = Script::from_source(source_code);
int result = script.call<int>("function_name", arg1, arg2);
```

### 2. Event<F> Template

Events for faster repeated calls:

```cpp
Event<int(int)> factorial{script, "factorial"};
int result = factorial(5);  // Faster than script.call()
```

### 3. Host Callbacks

Register host functions using `HostBindings::register_function()`:

```cpp
HostBindings::register_function(
    "int host_add(int a, int b)",
    [](loongarch::Machine&, int a, int b) -> int {
        return a + b;
    });

// All scripts can now call host_add()
Script script = Script::from_source(R"(
    extern "C" int test() {
        return host_add(10, 32);
    }
)");
```

**Important:** Host callbacks must take `Machine&` as their first parameter.

## API Reference

### Script Class

```cpp
// Constructors
Script(const std::string& elf_path, const ScriptOptions& options = {});
static Script from_source(const std::string& source_code, const ScriptOptions& options = {});
static Script from_file(const std::string& source_path, const ScriptOptions& options = {});

// Call guest functions
template<typename Ret = int, typename... Args>
Ret call(const std::string& function_name, Args&&... args);

// Access machine
loongarch::Machine& machine();
```

### Event<F> Template

```cpp
// Create event with function signature
Event<return_type(arg_types...)> event{script, "function_name"};

// Call like a regular function
return_type result = event(args...);

// Check if function exists
bool exists = event.exists();
```

### HostBindings

```cpp
// Register host function (call during initialization)
static void register_function(const std::string& signature, auto callback);

// Add custom type definitions to auto-generated header
static void append_header_content(const std::string& content);
```

## Working with Complex Types

### Passing std::string from Host to Guest

Use `GuestStdString` for parameters and `ScopedCppString` for temporary allocation:

```cpp
// Host callback receiving std::string from guest
HostBindings::register_function(
    "void log_message(const std::string& msg)",
    [](loongarch::Machine& machine, const loongarch::GuestStdString* msg) {
        std::string_view text = msg->to_view(machine);
        fmt::print("[LOG] {}\n", text);
    });

// Event receiving std::string
Event<void(loongarch::GuestStdString)> greet{script, "greet"};
loongarch::ScopedCppString name{script.machine(), "World!"};
greet(name);  // Passes string to guest
```

**Key points:**
- Guest functions take `const std::string&`
- Host callbacks receive `const loongarch::GuestStdString*`
- Use `ScopedCppString` to allocate temporary strings in guest memory

### Passing Custom Structs

For structs containing `std::string` or `std::vector`, you must:

1. Define the struct with guest types (`GuestStdString`, `GuestStdVector<T>`)
2. Implement `fix_addresses()` to update internal pointers
3. Use `ScopedArenaObject<T>` for automatic memory management

```cpp
// Register struct definition
HostBindings::append_header_content(R"(
    struct Dialogue {
        std::string speaker;
        std::vector<std::string> lines;
    };
)");

// Guest function
Script script = Script::from_source(R"(
    extern "C" void do_dialogue(const Dialogue& dlg) {
        log_message("Dialogue by " + dlg.speaker + ":");
        for (const auto& line : dlg.lines) {
            log_message("  " + line);
        }
    }
)");

// Host-side struct mirroring guest layout
struct Dialogue {
    loongarch::GuestStdString speaker;
    loongarch::GuestStdVector<loongarch::GuestStdString> lines;

    Dialogue(Machine& m, const std::string& spk, const std::vector<std::string>& lns)
        : speaker(m, spk), lines(m, lns) {}

    void fix_addresses(Machine& m, address_t self) {
        speaker.fix_addresses(m, self + offsetof(Dialogue, speaker));
        lines.fix_addresses(m, self + offsetof(Dialogue, lines));
    }
};

// Create and pass to guest
using namespace loongarch;
ScopedArenaObject<Dialogue> dlg{
    script.machine(),
    script.machine(),
    "Alice",
    std::vector<std::string>{"Hello!", "Welcome!"}
};

Event<void(ScopedArenaObject<Dialogue>)> do_dialogue{script, "do_dialogue"};
do_dialogue(dlg);
```
Scoped arena object are heap allocated and live until the end of the scope. Used to pass complex data to guests.

### The fix_addresses() Trait

When structs contain `std::string` or `std::vector`, their internal pointers must be updated after allocation:

```cpp
void fix_addresses(Machine& machine, address_t self) {
    // Update each member's internal pointers
    member1.fix_addresses(machine, self + offsetof(MyStruct, member1));
    member2.fix_addresses(machine, self + offsetof(MyStruct, member2));
}
```

This is required because:
1. Constructors initially create objects with the sub-objects guest-side address not yet determined
2. `fix_addresses()` updates sub-objects to point into guest memory offset from the higher level objects

For simple structs (and even vectors) this is not necessary. Strings that fit into SSO need to know their own address. Other types don't.

## Complete Examples

### Example 1: Basic Host Callbacks

```cpp
HostBindings::register_function(
    "void host_print(int value)",
    [](loongarch::Machine&, int value) {
        fmt::print("[HOST] {}\n", value);
    });

Script script = Script::from_source(R"(
    extern "C" int compute(int x) {
        host_print(x);
        return x * 2;
    }
)");

int result = script.call<int>("compute", 21);  // Prints 21, returns 42
```

### Example 2: Stateful Callbacks

```cpp
static struct UserState {
	int counter = 0;
} user_state;

HostBindings::register_function(
    "int get_counter()",
    [](loongarch::Machine& m) -> int {
		UserState& state = *m.get_userdata<Script>()->get_userdata<UserState>();
        return state.counter;
    });

HostBindings::register_function(
    "void increment_counter()",
    [](loongarch::Machine& m) {
		UserState& state = *m.get_userdata<Script>()->get_userdata<UserState>();
        state.counter++;
    });

Script script = Script::from_source(R"(
    extern "C" int test() {
        int before = get_counter();
        increment_counter();
        increment_counter();
        int after = get_counter();
        return after - before;  // Returns 2
    })");
// Set the value from Script::get_userdata<T>:
script.set_userdata<UserState>(&user_state);
```

### Example 3: String Handling

```cpp
HostBindings::register_function(
    "void log_message(const std::string& msg)",
    [](loongarch::Machine& machine, const loongarch::GuestStdString* msg) {
        fmt::print("[LOG] {}\n", msg->to_view(machine));
    });

Script script = Script::from_source(R"(
    extern "C" void greet(const std::string& name) {
        log_message("Hello, " + name);
    }
)");

Event<void(loongarch::GuestStdString)> greet{script, "greet"};
loongarch::ScopedCppString name{script.machine(), "World"};

greet(name);  // Prints "[LOG] Hello, World"
```

## Technical Details

### How Host Callbacks Work

1. Guest code declares extern "C" functions
2. `HostBindings::register_function()` registers them globally at initialization
3. During compilation, the framework generates a header with declarations
4. The linker resolves calls to thin wrapper functions
5. At runtime, wrappers execute a bytecode that executes a callback in the Script instance
6. Arguments are extracted from LoongArch ABI registers (A0-A7, FA0-FA7)
7. Return values are placed in A0/FA0

## Building

```bash
cd examples/script
mkdir build && cd build
cmake ../..
make script_example
./script_example
```

## Requirements

- C++20 compiler (GCC 11+, Clang 14+)
- CMake 3.14+
- LoongArch cross-compiler (`loongarch64-linux-gnu-g++`)
- libloong library

## Pitfalls

1. **Forgetting Machine& parameter**: All host callbacks must take `Machine&` as first argument
2. **Not calling fix_addresses()**: Required for structs with `std::string`/`std::vector<std::string>`
3. **Mixing host/guest types**: Use `GuestStdString`, not `std::string` in host-side structs
4. **Memory lifetime**: Guest memory references become invalid after Script destruction
