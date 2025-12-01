#pragma once
#include <libloong/machine.hpp>
#include <vector>
#include <string>
#include <functional>
#include <sstream>

namespace loongarch::test {

struct ExecutionResult {
	bool success = false;
	int exit_code = -1;
	uint64_t instructions_executed = 0;
	std::string error;
	std::string stdout_output;
	address_t final_pc = 0;
	bool reached_main = false;
};

class TestMachine {
public:
	TestMachine(const std::vector<uint8_t>& binary, uint64_t max_memory = 256 * 1024 * 1024)
		: m_binary_data(binary)
	{
		std::string_view binary_view(
			reinterpret_cast<const char*>(m_binary_data.data()),
			m_binary_data.size());

		MachineOptions options;
		options.verbose_loader = false;
		options.verbose_syscalls = false;
		options.memory_max = max_memory;

		m_machine = std::make_unique<Machine>(binary_view, options);
	}

	TestMachine(std::string_view binary_view, uint64_t max_memory = 256 * 1024 * 1024) {
		MachineOptions options;
		options.verbose_loader = false;
		options.verbose_syscalls = false;
		options.memory_max = max_memory;

		m_machine = std::make_unique<Machine>(binary_view, options);
	}

	// Setup standard Linux environment
	void setup_linux(
		const std::vector<std::string>& args = {"test_program"},
		const std::vector<std::string>& env = {"LC_ALL=C", "USER=test"})
	{
		m_machine->setup_linux_syscalls();
		m_machine->setup_linux(args, env);

		// Set up exit address for vmcalls
		// Try to find fast_exit, _exit, or other common exit functions
		auto exit_addr = m_machine->address_of("fast_exit");
		if (exit_addr == 0) {
			exit_addr = m_machine->address_of("_exit");
		}
		if (exit_addr == 0) {
			exit_addr = m_machine->address_of("__exit");
		}
		if (exit_addr != 0) {
			m_machine->memory.set_exit_address(exit_addr);
		}

		// Run program initialization to resolve IFUNCs, then reset to start
		// This is critical for libc functions like strcmp that use IFUNC
		// to select optimized implementations (e.g., LSX versions)
		m_initialized = false;
	}

	// Setup minimal environment (no syscalls)
	void setup_minimal() {
		m_machine->setup_minimal_syscalls();
	}

	// Execute the program
	ExecutionResult execute(uint64_t max_instructions = 10'000'000) {
		ExecutionResult result;

		try {
			m_machine->simulate(max_instructions);
			result.instructions_executed = m_machine->instruction_counter();
			result.final_pc = m_machine->cpu.pc();

			if (m_machine->stopped()) {
				result.exit_code = static_cast<int>(m_machine->cpu.reg(REG_A0));
				result.success = true;
			} else {
				result.error = "Program did not complete within instruction limit";
			}

			// Check if main was reached
			auto main_addr = m_machine->address_of("main");
			if (main_addr != 0) {
				result.reached_main = (result.final_pc >= main_addr &&
				                       result.final_pc <= main_addr + 0x1000);
			}

		} catch (const MachineException& e) {
			result.error = std::string("MachineException: ") + e.what() +
			               " (type=" + std::to_string(static_cast<int>(e.type())) + ")";
			result.final_pc = e.data();
			result.instructions_executed = m_machine->instruction_counter();
		} catch (const std::exception& e) {
			result.error = std::string("Exception: ") + e.what();
			result.instructions_executed = m_machine->instruction_counter();
		}

		return result;
	}

	// Call a function in the guest (returns full 64-bit result)
	template <typename... Args>
	address_t vmcall(const std::string& func_name, Args&&... args) {
		auto func_addr = m_machine->address_of(func_name);
		if (func_addr == 0) {
			throw std::runtime_error("Function not found: " + func_name);
		}
		return vmcall(func_addr, std::forward<Args>(args)...);
	}

	template <typename... Args>
	address_t vmcall(address_t func_addr, Args&&... args) {
		// Ensure IFUNCs are resolved before first vmcall
		// This runs program initialization which resolves IFUNC symbols
		// like strcmp that select optimized implementations at runtime
		ensure_initialized();
		return m_machine->vmcall(func_addr, std::forward<Args>(args)...);
	}

	// Direct access to machine
	Machine& machine() { return *m_machine; }
	const Machine& machine() const { return *m_machine; }

	// Read/write guest memory
	template <typename T>
	T read(address_t addr) {
		return m_machine->memory.template read<T>(addr);
	}

	template <typename T>
	void write(address_t addr, T value) {
		m_machine->memory.template write<T>(addr, value);
	}

	// Get register value
	uint64_t get_reg(int reg) const {
		return m_machine->cpu.reg(reg);
	}

	// Set register value
	void set_reg(int reg, uint64_t value) {
		m_machine->cpu.reg(reg) = value;
	}

	// Symbol lookup
	address_t address_of(const std::string& name) const {
		return m_machine->address_of(name);
	}

private:
	void ensure_initialized() {
		if (m_initialized) {
			return;
		}

		// Save initial state
		auto saved_regs = m_machine->cpu.registers();

		// Run initialization to resolve IFUNCs
		m_machine->simulate(10'000'000ull);

		// Restore initial state so program can run normally
		m_machine->cpu.registers() = saved_regs;
		m_machine->set_instruction_counter(0);

		m_initialized = true;
	}

	std::vector<uint8_t> m_binary_data;
	std::unique_ptr<Machine> m_machine;
	bool m_initialized = false;
};

// Helper function to run a simple test
inline ExecutionResult run_binary(
	const std::vector<uint8_t>& binary,
	int expected_exit_code = 0,
	uint64_t max_instructions = 10'000'000ull)
{
	TestMachine machine(binary);
	machine.setup_linux();
	auto result = machine.execute(max_instructions);

	if (result.success && result.exit_code == expected_exit_code) {
		result.success = true;
	} else if (result.success) {
		result.success = false;
		result.error = "Exit code mismatch: expected " +
		               std::to_string(expected_exit_code) +
		               ", got " + std::to_string(result.exit_code);
	}

	return result;
}

// Helper to create simple C programs on the fly
inline std::string make_c_program(const std::string& body) {
	return "#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n\n" + body;
}

inline std::string make_cpp_program(const std::string& body) {
	return "#include <iostream>\n#include <cstdlib>\n#include <cstring>\n\n" + body;
}

// Helper for bare metal programs (no libc)
inline std::string make_bare_program(const std::string& body) {
	return R"(
void _start() {
	asm volatile(
		"li.d $a7, 93\n"       // __NR_exit
		"li.d $a0, %0\n"       // exit code
		"syscall 0\n"
		: : "i"(42)
	);
	__builtin_unreachable();
}
)" + body;
}

} // namespace loongarch::test
