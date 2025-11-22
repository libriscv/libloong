#include "debug.hpp"
#include <cstdio>
#include <cxxabi.h>
#include <memory>
#include <string>

namespace loongarch
{
	template <int W>
	void DebugMachine<W>::simulate(uint64_t max_instructions)
	{
		this->machine.set_max_instructions(max_instructions);
		while (!this->machine.stopped()) {
			if (this->verbose_instructions)
				print_instruction();
			this->machine.cpu.step_one();
			if (this->machine.cpu.reg(0) != 0) {
				print_registers();
				throw MachineException(PROTECTION_FAULT, "Zero register R0 modified");
			}
			if (this->verbose_registers)
				print_registers();
		}
	}

	template <int W>
	long DebugMachine<W>::vmcall(
		address_type<W> func_addr,
		uint64_t max_instructions,
		const std::vector<std::string>& arguments)
	{
		// Find an exit function that will terminate the vmcall
		auto exit_addr = machine.address_of("fast_exit");
		if (exit_addr == 0) {
			exit_addr = machine.address_of("_exit");
			if (exit_addr == 0) {
				throw std::runtime_error("No fast_exit/_exit function found for vmcall");
			}
		}
		this->machine.memory.set_exit_address(exit_addr);
		this->machine.cpu.reg(REG_RA) = exit_addr;
		// Set PC to function address
		this->machine.cpu.jump(func_addr);
		// Push arguments onto stack
		address_type<W> sp = this->machine.memory.stack_address();
		int reg = 0;
		for (const auto& argument : arguments) {
			address_type<W> arg_addr = this->machine.stack_push(sp, argument.c_str(), argument.size() + 1);
			this->machine.cpu.reg(REG_A0 + reg++) = arg_addr;
		}
		this->machine.cpu.reg(REG_SP) = sp;
		// Simulate with debugging
		simulate(max_instructions);
		// Return the function return value (if any)
		return this->machine.template return_value<long>();
	}

	template <int W>
	void DebugMachine<W>::print_registers()
	{
		for (int i = 0; i < 32; i += 4) {
			printf("R%02d: 0x%016lx  R%02d: 0x%016lx  R%02d: 0x%016lx  R%02d: 0x%016lx\n",
				i,   (unsigned long)machine.cpu.reg(i),
				i+1, (unsigned long)machine.cpu.reg(i+1),
				i+2, (unsigned long)machine.cpu.reg(i+2),
				i+3, (unsigned long)machine.cpu.reg(i+3));
		}
	}

	template <int W>
	void DebugMachine<W>::print_instruction()
	{
		auto pc = machine.cpu.pc();
		const auto* symbol = machine.lookup_symbol(pc);
		std::string objdump_line;
		if (this->compare_objdump && symbol) {
			// Check cache first
			auto it = m_objdump_cache.find(pc);
			if (it != m_objdump_cache.end()) {
				objdump_line = it->second;
			} else {
				objdump_line = get_objdump_line(pc);
				m_objdump_cache.insert_or_assign(pc, objdump_line);
			}
		}

		if (this->short_output) {
			printf("%s%s\n",
				machine.cpu.current_instruction_to_string().c_str(),
				objdump_line.c_str());
		}
		else if (symbol) {
			auto demangled = demangle(symbol->name.c_str());
			auto offset = pc - symbol->address;
			printf("PC: 0x%lx  [%s+0x%lx]  %s%s\n",
				(unsigned long)pc,
				demangled.c_str(),
				(unsigned long)offset,
				machine.cpu.current_instruction_to_string().c_str(),
				objdump_line.c_str());
		} else {
			printf("PC: 0x%lx  %s%s\n",
				(unsigned long)pc,
				machine.cpu.current_instruction_to_string().c_str(),
				objdump_line.c_str());
		}
	}

	template <int W>
	std::string DebugMachine<W>::demangle(const char* mangled)
	{
		int status;
		std::unique_ptr<char, void(*)(void*)> demangled(
			abi::__cxa_demangle(mangled, nullptr, nullptr, &status),
			std::free
		);

		if (status == 0 && demangled) {
			return std::string(demangled.get());
		}
		return std::string(mangled);
	}

	template <int W>
	std::string DebugMachine<W>::get_objdump_line(const address_type<W> pc)
	{
		// Attempt to get objdump line for current PC
		// Note: This requires the binary to have debug symbols
		// Run objdump command
		char buffer[512];
		snprintf(buffer, sizeof(buffer), "%s -d --start-address=0x%lx --stop-address=0x%lx %s",
			this->objdump_path.c_str(),
			(unsigned long)pc,
			(unsigned long)(pc + 4),
			this->filename.c_str());

		std::string objdump_line;
		FILE* pipe = popen(buffer, "r");
		if (pipe) {
			// Re-using buffer to read output
			while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
				objdump_line += buffer;
			}
			pclose(pipe);
		}
		// Extract only the *last* line (the relevant instruction)
		size_t last_newline = objdump_line.find_last_of('\n', objdump_line.length() - 2);
		if (last_newline != std::string::npos) {
			objdump_line = objdump_line.substr(last_newline + 1);
		}
		// And only after the second column (skip address and bytes)
		size_t second_col = objdump_line.find('\t');
		if (second_col != std::string::npos) {
			second_col = objdump_line.find('\t', second_col + 1);
			if (second_col != std::string::npos) {
				return "  ;; ACTUAL: " + objdump_line.substr(second_col + 1);
			}
		}
		return "";
	}

#ifdef LA_32
	template struct DebugMachine<LA32>;
#endif
#ifdef LA_64
	template struct DebugMachine<LA64>;
#endif
} // loongarch
