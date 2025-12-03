#pragma once

#include <libloong/machine.hpp>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <stdexcept>
#include <functional>
#include <unordered_map>
#include "host_bindings.hpp"

namespace loongarch::script {

// Forward declarations
class HostCallbackManager;

class ScriptException : public std::runtime_error {
public:
    explicit ScriptException(const std::string& msg) : std::runtime_error(msg) {}
};

class CompilationException : public ScriptException {
public:
    explicit CompilationException(const std::string& msg) : ScriptException(msg) {}
};

class ExecutionException : public ScriptException {
public:
    explicit ExecutionException(const std::string& msg) : ScriptException(msg) {}
};

struct ScriptOptions {
    // Machine options
    size_t memory_max = 256 * 1024 * 1024;  // 256 MB
    size_t stack_size = 2 * 1024 * 1024;    // 2 MB
    size_t brk_size = 1 * 1024 * 1024;      // 1 MB

    // Compilation options
    std::string compiler = "loongarch64-linux-gnu-g++-14";
    std::vector<std::string> compile_flags = {"-O2", "-std=c++20"};
    std::vector<std::string> link_flags = {"-static", "-Wl,-Ttext-segment=0x200000"};

    // Runtime options
    bool verbose = false;
    uint64_t max_instructions = 0;  // 0 = unlimited

    // Temporary file handling
    std::string temp_dir = "/tmp";
    bool keep_temp_files = false;
};

class Script {
public:
    // Constructor: Load from existing ELF binary
    explicit Script(const std::string& elf_path, const ScriptOptions& options = {});

    // Constructor: Compile from C++ source code
    static Script from_source(const std::string& source_code, const ScriptOptions& options = {});

    // Constructor: Compile from C++ source file
    static Script from_file(const std::string& source_path, const ScriptOptions& options = {});

    // Call a guest function with arguments
    template<typename Ret = int, typename... Args>
    Ret call(address_t addr, Args&&... args) {
        try {
			if constexpr (std::is_void_v<Ret>) {
				m_machine->template vmcall<UINT64_MAX>(
					addr, std::forward<Args>(args)...
				);
			} else {
				return m_machine->template vmcall<UINT64_MAX>(
					addr, std::forward<Args>(args)...
				);
			}
        } catch (const MachineException& e) {
			handle_exception(e);
        }
    }

	template<typename Ret = int, typename... Args>
    Ret call(const std::string& function_name, Args&&... args) {
		const address_t addr = address_of(function_name);
		return call<Ret>(addr, std::forward<Args>(args)...);
    }

    // Check if a function exists
    bool has_function(const std::string& function_name) const;

    // Get address of a symbol
    address_t address_of(const std::string& symbol_name) const;

	// Set and get user data pointer
	template <typename T>
	void set_userdata(T* data) { m_userdata = data; }
	template <typename T>
	T* get_userdata() const noexcept { return static_cast<T*> (m_userdata); }

    // Access to underlying machine (advanced users)
    Machine& machine() { return *m_machine; }
    const Machine& machine() const { return *m_machine; }

    // Memory access helpers
    template<typename T>
    T read_memory(address_t addr) const {
        return m_machine->memory.template read<T>(addr);
    }

    template<typename T>
    void write_memory(address_t addr, const T& value) {
        m_machine->memory.template write<T>(addr, value);
    }

    std::string read_string(address_t addr, size_t max_len = 4096) const {
        return m_machine->memory.memstring(addr, max_len);
    }

    Script(Script&&) = default;
    Script& operator=(Script&&) = default;
    Script(const Script&) = delete;
    Script& operator=(const Script&) = delete;
    ~Script();
private:
    Script(std::vector<uint8_t> binary, const ScriptOptions& options);
	[[noreturn]] void handle_exception(const MachineException& e) const;

    // Compile C++ source to ELF binary
    static std::vector<uint8_t> compile_source(
        const std::string& source_code,
        const ScriptOptions& options,
        std::string& temp_file_path
    );

    // Load ELF file into memory
    static std::vector<uint8_t> load_elf_file(const std::string& path);

    // Initialize the machine
    void initialize_machine();

    // Patch registered host functions into the guest
    void patch_host_functions();

    // Callback dispatcher (static)
    static void dispatch_callback(Machine& machine, int syscall_num);

    std::unique_ptr<Machine> m_machine;
	void* m_userdata = nullptr;
    std::vector<uint8_t> m_binary;
    ScriptOptions m_options;
    std::string m_temp_file;
};

// Event class: Cached host -> guest function calls
template<typename F>
class Event {
public:
    using func_type = std::function<F>;
    using traits = function_traits<F>;
    using return_type = typename traits::return_type;

    Event(Script& script, const std::string& function_name)
        : m_script(script), m_function_name(function_name), m_address(0)
    {
		m_address = m_script.address_of(m_function_name);
		if (m_address == 0x0) {
			throw ScriptException("Function not found: " + m_function_name);
		}
    }

    // Call operator - invokes the guest function
    template<typename... Args>
    return_type operator()(Args&&... args) {
        if constexpr (std::is_void_v<return_type>) {
            m_script.call(m_address, std::forward<Args>(args)...);
        } else {
            return m_script.call<return_type>(m_address, std::forward<Args>(args)...);
        }
    }

private:
    Script& m_script;
    const std::string m_function_name;
    mutable address_t m_address;
};

} // namespace loongarch::script
