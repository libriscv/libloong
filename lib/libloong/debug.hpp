#pragma once
#include "machine.hpp"
#include <unordered_map>

namespace loongarch
{
	// Debug machine wrapper for stepping and inspection
	template <int W>
	struct DebugMachine {
		Machine<W>& machine;
		bool verbose_registers = true;
		bool verbose_instructions = true;
		bool compare_objdump = false;
		bool stop_on_objdump_mismatch = false;
		bool short_output = false;
		std::string filename;
		std::string objdump_path = "loongarch64-linux-gnu-objdump";

		DebugMachine(Machine<W>& m) : machine(m) {}

		void simulate(uint64_t max_instructions = 1'000'000ull);
		// Argument-less VM function call with debugging
		long vmcall(address_type<W> func_addr,
			uint64_t max_instructions = 1'000'000ull,
			const std::vector<std::string>& arguments = {});
		void print_registers();
		void print_instruction();
		std::string demangle(const char* mangled);
	private:
		std::string get_objdump_line(address_type<W> pc);
		bool compare_instructions(const std::string& our_instr, const std::string& objdump_instr);
		std::unordered_map<address_type<W>, std::string> m_objdump_cache;
	};

} // namespace loongarch
