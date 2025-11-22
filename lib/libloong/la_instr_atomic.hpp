#pragma once
#include "cpu.hpp"
#include "la_instr.hpp"

namespace loongarch
{
	// Template implementations for atomic memory operations
	
	template <int W>
	struct AtomicImpl {
		using cpu_t = CPU<W>;
		using addr_t = address_type<W>;
		
		// AMSWAP.W: Atomic memory swap (32-bit)
		// Format: amswap.w rd, rk, rj
		// Operation:
		//   temp = MEM[rj]
		//   MEM[rj] = rk
		//   rd = sign_extend(temp)
		static void AMSWAP_W(cpu_t& cpu, la_instruction instr) {
			addr_t addr = cpu.reg(instr.r3.rj);
			uint32_t old_value = cpu.memory().template read<uint32_t>(addr);
			uint32_t new_value = (uint32_t)cpu.reg(instr.r3.rk);
			cpu.memory().template write<uint32_t>(addr, new_value);
			// Sign-extend the 32-bit old value to 64 bits
			if (instr.r3.rd != 0) // Writes to x0 are discarded
				cpu.reg(instr.r3.rd) = (int64_t)(int32_t)old_value;
		}

		// AMSWAP.D: Atomic memory swap (64-bit)
		// Format: amswap.d rd, rk, rj
		// Operation:
		//   temp = MEM[rj]
		//   MEM[rj] = rk
		//   rd = temp
		static void AMSWAP_D(cpu_t& cpu, la_instruction instr) {
			addr_t addr = cpu.reg(instr.r3.rj);
			uint64_t old_value = cpu.memory().template read<uint64_t>(addr);
			uint64_t new_value = cpu.reg(instr.r3.rk);
			cpu.memory().template write<uint64_t>(addr, new_value);
			if (instr.r3.rd != 0) // Writes to x0 are discarded
				cpu.reg(instr.r3.rd) = old_value;
		}
	};
	
	// Template printers for atomic operations
	
	template <int W>
	struct AtomicPrinters {
		using cpu_t = CPU<W>;
		using addr_t = address_type<W>;
		
		static const char* reg_name(uint32_t reg) {
			static const char* names[] = {
				"$zero", "$ra", "$tp", "$sp", "$a0", "$a1", "$a2", "$a3",
				"$a4", "$a5", "$a6", "$a7", "$t0", "$t1", "$t2", "$t3",
				"$t4", "$t5", "$t6", "$t7", "$t8", "$r21", "$fp", "$s0",
				"$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$s8"
			};
			return (reg < 32) ? names[reg] : "?";
		}
		
	static int AMSWAP_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		// Memory ordering bits [17:16]: 00=none, 01=_aq, 10=_rl, 11=_db (acq_rel)
		uint32_t ord = (instr.whole >> 16) & 0x3;
		const char* suffix = (ord == 0) ? "" : (ord == 1) ? "_aq" : (ord == 2) ? "_rl" : "_db";
		return snprintf(buf, len, "amswap%s.w %s, %s, %s", suffix,
			reg_name(instr.r3.rd), reg_name(instr.r3.rk), reg_name(instr.r3.rj));
	}
	
	static int AMSWAP_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		// Memory ordering bits [17:16]: 00=none, 01=_aq, 10=_rl, 11=_db (acq_rel)
		uint32_t ord = (instr.whole >> 16) & 0x3;
		const char* suffix = (ord == 0) ? "" : (ord == 1) ? "_aq" : (ord == 2) ? "_rl" : "_db";
		return snprintf(buf, len, "amswap%s.d %s, %s, %s", suffix,
			reg_name(instr.r3.rd), reg_name(instr.r3.rk), reg_name(instr.r3.rj));
	}
};} // namespace loongarch
