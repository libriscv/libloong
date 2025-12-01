#include "debug.hpp"
#include <cstdio>
#include <cxxabi.h>
#include <memory>
#include <string>

namespace loongarch
{
	void DebugMachine::simulate(uint64_t max_instructions)
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

	long DebugMachine::vmcall(
		address_t func_addr,
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
		address_t sp = this->machine.memory.stack_address();
		int reg = 0;
		for (const auto& argument : arguments) {
			address_t arg_addr = this->machine.stack_push(sp, argument.c_str(), argument.size() + 1);
			this->machine.cpu.reg(REG_A0 + reg++) = arg_addr;
		}
		this->machine.cpu.reg(REG_SP) = sp;
		// Simulate with debugging
		simulate(max_instructions);
		// Return the function return value (if any)
		return this->machine.template return_value<long>();
	}

	void DebugMachine::print_registers()
	{
		printf("%s\n", machine.cpu.registers().to_string().c_str());
	}

	void DebugMachine::print_instruction()
	{
		auto pc = machine.cpu.pc();
		const auto* symbol = machine.lookup_symbol(pc);
		std::string objdump_line;
		std::string our_instr = machine.cpu.current_instruction_to_string();

		if (this->compare_objdump && symbol) {
			// Check cache first
			auto it = m_objdump_cache.find(pc);
			if (it != m_objdump_cache.end()) {
				objdump_line = it->second;
			} else {
				objdump_line = get_objdump_line(pc);
				m_objdump_cache.insert_or_assign(pc, objdump_line);
			}

			// Check for mismatch if stop_on_objdump_mismatch is enabled
			if (this->stop_on_objdump_mismatch && !objdump_line.empty()) {
				if (!compare_instructions(our_instr, objdump_line)) {
					printf("\n*** INSTRUCTION MNEMONIC MISMATCH DETECTED ***\n");
					printf("PC: 0x%lx\n", (unsigned long)pc);
					printf("Our output:  %s\n", our_instr.c_str());
					printf("Objdump:     %s", objdump_line.c_str());
					if (symbol) {
						auto demangled = demangle(symbol->name.c_str());
						auto offset = pc - symbol->address;
						printf("In function: %s+0x%lx\n", demangled.c_str(), (unsigned long)offset);
					}
					printf("\n");
					throw MachineException(ILLEGAL_OPERATION, "Instruction mnemonic mismatch with objdump");
				}
			}
		}

		if (this->short_output) {
			printf("%s%s\n",
				our_instr.c_str(),
				objdump_line.c_str());
		}
		else if (symbol) {
			auto demangled = demangle(symbol->name.c_str());
			auto offset = pc - symbol->address;
			printf("PC: 0x%lx  [%s+0x%lx]  %s%s\n",
				(unsigned long)pc,
				demangled.c_str(),
				(unsigned long)offset,
				our_instr.c_str(),
				objdump_line.c_str());
		} else {
			printf("PC: 0x%lx  %s%s\n",
				(unsigned long)pc,
				our_instr.c_str(),
				objdump_line.c_str());
		}
	}

	std::string DebugMachine::demangle(const char* mangled)
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

	std::string DebugMachine::get_objdump_line(const address_t pc)
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

	bool DebugMachine::compare_instructions(const std::string& our_instr, const std::string& objdump_instr)
	{
		// Extract the actual instruction from objdump output (remove ";; ACTUAL: " prefix)
		std::string objdump_clean;
		const std::string prefix = "  ;; ACTUAL: ";
		if (objdump_instr.find(prefix) == 0) {
			objdump_clean = objdump_instr.substr(prefix.length());
		} else {
			objdump_clean = objdump_instr;
		}

		// Helper to normalize and parse instruction into mnemonic and operands
		auto parse_instruction = [](const std::string& s) -> std::pair<std::string, std::vector<std::string>> {
			// Trim whitespace
			size_t start = s.find_first_not_of(" \t\r\n");
			size_t end = s.find_last_not_of(" \t\r\n");
			if (start == std::string::npos || end == std::string::npos) {
				return {"", {}};
			}
			std::string clean = s.substr(start, end - start + 1);

			// Split mnemonic from operands
			size_t space_pos = clean.find_first_of(" \t");
			std::string mnemonic;
			std::string operands_str;

			if (space_pos != std::string::npos) {
				mnemonic = clean.substr(0, space_pos);
				// Skip whitespace after mnemonic
				size_t operands_start = clean.find_first_not_of(" \t", space_pos);
				if (operands_start != std::string::npos) {
					operands_str = clean.substr(operands_start);
				}
			} else {
				mnemonic = clean;
			}

			// Parse operands by splitting on commas
			std::vector<std::string> operands;
			if (!operands_str.empty()) {
				size_t pos = 0;
				while (pos < operands_str.length()) {
					size_t comma = operands_str.find(',', pos);
					std::string operand;
					if (comma != std::string::npos) {
						operand = operands_str.substr(pos, comma - pos);
						pos = comma + 1;
					} else {
						operand = operands_str.substr(pos);
						pos = operands_str.length();
					}
					// Trim whitespace from operand
					size_t op_start = operand.find_first_not_of(" \t");
					size_t op_end = operand.find_last_not_of(" \t");
					if (op_start != std::string::npos && op_end != std::string::npos) {
						operands.push_back(operand.substr(op_start, op_end - op_start + 1));
					}
				}
			}

			return {mnemonic, operands};
		};

		auto [our_mnemonic, our_operands] = parse_instruction(our_instr);
		auto [objdump_mnemonic, objdump_operands] = parse_instruction(objdump_clean);

		// Compare mnemonics (case-sensitive for LoongArch)
		if (our_mnemonic != objdump_mnemonic) {
			// Instruction mismatch - this is an error
			return false;
		}

		// Compare operands count
		if (our_operands.size() != objdump_operands.size()) {
			// Different number of operands - warn but don't error
			printf("*** WARNING: Operand count mismatch (ours: %zu, objdump: %zu) ***\n",
				our_operands.size(), objdump_operands.size());
			// Still return true to continue - this might be formatting difference
		} else {
			// Check each operand for differences
			bool has_operand_diff = false;
			for (size_t i = 0; i < our_operands.size(); i++) {
				if (our_operands[i] != objdump_operands[i]) {
					if (!has_operand_diff) {
						printf("*** WARNING: Operand formatting differences detected ***\n");
						has_operand_diff = true;
					}
					printf("  Operand %zu: '%s' vs '%s'\n", i, our_operands[i].c_str(), objdump_operands[i].c_str());
				}
			}
		}

		// Return true if mnemonic matches (operand differences are just warnings)
		return true;
	}
// Removed template instantiation
} // loongarch
