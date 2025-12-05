#pragma once
#include "machine.hpp"
#include <unordered_map>

namespace loongarch
{
	// Debug machine wrapper for stepping and inspection
	struct DebugMachine {
		Machine& machine;
		bool verbose_registers = true;
		bool verbose_instructions = true;
		bool compare_objdump = false;
		bool stop_on_objdump_mismatch = false;
		bool short_output = false;
		std::string filename;
		std::string objdump_path = "loongarch64-linux-gnu-objdump";

		DebugMachine(Machine& m) : machine(m) {}

		void simulate(uint64_t max_instructions = 1'000'000ull);
		// Argument-less VM function call with debugging
		long vmcall(address_t func_addr,
			uint64_t max_instructions = 1'000'000ull,
			const std::vector<std::string>& arguments = {});
		// Decode-only mode: decode all instructions and compare with objdump without execution
		void decode_and_compare();
		void print_registers();
		void print_instruction();
		std::string demangle(const char* mangled);
	private:
		std::string get_objdump_line(address_t pc);
		bool compare_instructions(const std::string& our_instr, const std::string& objdump_instr);
		std::unordered_map<address_t, std::string> m_objdump_cache;
	};

} // namespace loongarch
