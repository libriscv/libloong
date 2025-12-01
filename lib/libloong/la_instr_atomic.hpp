#pragma once
#include "cpu.hpp"
#include "la_instr.hpp"

namespace loongarch
{
	// Template implementations for atomic memory operations

	struct AtomicImpl {
		using cpu_t = CPU;
		using addr_t = address_t;

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

		// AMADD.W: Atomic memory add (32-bit)
		// Format: amadd.w rd, rk, rj
		// Operation:
		//   temp = MEM[rj]
		//   MEM[rj] = temp + rk
		//   rd = sign_extend(temp)
		static void AMADD_W(cpu_t& cpu, la_instruction instr) {
			addr_t addr = cpu.reg(instr.r3.rj);
			uint32_t old_value = cpu.memory().template read<uint32_t>(addr);
			uint32_t add_value = (uint32_t)cpu.reg(instr.r3.rk);
			uint32_t new_value = old_value + add_value;
			cpu.memory().template write<uint32_t>(addr, new_value);
			// Sign-extend the 32-bit old value to 64 bits
			if (instr.r3.rd != 0) // Writes to x0 are discarded
				cpu.reg(instr.r3.rd) = (int64_t)(int32_t)old_value;
		}

		// AMADD.D: Atomic memory add (64-bit)
		// Format: amadd.d rd, rk, rj
		// Operation:
		//   temp = MEM[rj]
		//   MEM[rj] = temp + rk
		//   rd = temp
		static void AMADD_D(cpu_t& cpu, la_instruction instr) {
			addr_t addr = cpu.reg(instr.r3.rj);
			uint64_t old_value = cpu.memory().template read<uint64_t>(addr);
			uint64_t add_value = cpu.reg(instr.r3.rk);
			uint64_t new_value = old_value + add_value;
			cpu.memory().template write<uint64_t>(addr, new_value);
			if (instr.r3.rd != 0) // Writes to x0 are discarded
				cpu.reg(instr.r3.rd) = old_value;
		}

		// AMAND.W: Atomic memory AND (32-bit)
		static void AMAND_W(cpu_t& cpu, la_instruction instr) {
			addr_t addr = cpu.reg(instr.r3.rj);
			uint32_t old_value = cpu.memory().template read<uint32_t>(addr);
			uint32_t and_value = (uint32_t)cpu.reg(instr.r3.rk);
			uint32_t new_value = old_value & and_value;
			cpu.memory().template write<uint32_t>(addr, new_value);
			if (instr.r3.rd != 0)
				cpu.reg(instr.r3.rd) = (int64_t)(int32_t)old_value;
		}

		// AMAND.D: Atomic memory AND (64-bit)
		static void AMAND_D(cpu_t& cpu, la_instruction instr) {
			addr_t addr = cpu.reg(instr.r3.rj);
			uint64_t old_value = cpu.memory().template read<uint64_t>(addr);
			uint64_t and_value = cpu.reg(instr.r3.rk);
			uint64_t new_value = old_value & and_value;
			cpu.memory().template write<uint64_t>(addr, new_value);
			if (instr.r3.rd != 0)
				cpu.reg(instr.r3.rd) = old_value;
		}

		// AMOR.W: Atomic memory OR (32-bit)
		static void AMOR_W(cpu_t& cpu, la_instruction instr) {
			addr_t addr = cpu.reg(instr.r3.rj);
			uint32_t old_value = cpu.memory().template read<uint32_t>(addr);
			uint32_t or_value = (uint32_t)cpu.reg(instr.r3.rk);
			uint32_t new_value = old_value | or_value;
			cpu.memory().template write<uint32_t>(addr, new_value);
			if (instr.r3.rd != 0)
				cpu.reg(instr.r3.rd) = (int64_t)(int32_t)old_value;
		}

		// AMOR.D: Atomic memory OR (64-bit)
		static void AMOR_D(cpu_t& cpu, la_instruction instr) {
			addr_t addr = cpu.reg(instr.r3.rj);
			uint64_t old_value = cpu.memory().template read<uint64_t>(addr);
			uint64_t or_value = cpu.reg(instr.r3.rk);
			uint64_t new_value = old_value | or_value;
			cpu.memory().template write<uint64_t>(addr, new_value);
			if (instr.r3.rd != 0)
				cpu.reg(instr.r3.rd) = old_value;
		}

		// AMXOR.W: Atomic memory XOR (32-bit)
		static void AMXOR_W(cpu_t& cpu, la_instruction instr) {
			addr_t addr = cpu.reg(instr.r3.rj);
			uint32_t old_value = cpu.memory().template read<uint32_t>(addr);
			uint32_t xor_value = (uint32_t)cpu.reg(instr.r3.rk);
			uint32_t new_value = old_value ^ xor_value;
			cpu.memory().template write<uint32_t>(addr, new_value);
			if (instr.r3.rd != 0)
				cpu.reg(instr.r3.rd) = (int64_t)(int32_t)old_value;
		}

		// AMXOR.D: Atomic memory XOR (64-bit)
		static void AMXOR_D(cpu_t& cpu, la_instruction instr) {
			addr_t addr = cpu.reg(instr.r3.rj);
			uint64_t old_value = cpu.memory().template read<uint64_t>(addr);
			uint64_t xor_value = cpu.reg(instr.r3.rk);
			uint64_t new_value = old_value ^ xor_value;
			cpu.memory().template write<uint64_t>(addr, new_value);
			if (instr.r3.rd != 0)
				cpu.reg(instr.r3.rd) = old_value;
		}
	};

	// Template printers for atomic operations

	struct AtomicPrinters {
		using cpu_t = CPU;
		using addr_t = address_t;

		static const char* reg_name(uint32_t reg) {
			static const char* names[] = {
				"$zero", "$ra", "$tp", "$sp", "$a0", "$a1", "$a2", "$a3",
				"$a4", "$a5", "$a6", "$a7", "$t0", "$t1", "$t2", "$t3",
				"$t4", "$t5", "$t6", "$t7", "$t8", "$r21", "$fp", "$s0",
				"$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$s8"
			};
			return (reg < 32) ? names[reg] : "?";
		}

	// Helper to decode atomic ordering suffix
	// Empirical encoding in bits[19:16]:
	// - op with no ordering: 0-7
	// - op with _db: op + 9
	static const char* get_atomic_suffix(uint32_t whole) {
		uint32_t op_ord = (whole >> 16) & 0xF;
		// If op_ord >= 9, it has _db ordering
		return (op_ord >= 9) ? "_db" : "";
		// TODO: Determine encodings for _aq and _rl when encountered
	}

	static int AMSWAP_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "amswap%s.w %s, %s, %s", get_atomic_suffix(instr.whole),
			reg_name(instr.r3.rd), reg_name(instr.r3.rk), reg_name(instr.r3.rj));
	}

	static int AMSWAP_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "amswap%s.d %s, %s, %s", get_atomic_suffix(instr.whole),
			reg_name(instr.r3.rd), reg_name(instr.r3.rk), reg_name(instr.r3.rj));
	}

	static int AMADD_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "amadd%s.w %s, %s, %s", get_atomic_suffix(instr.whole),
			reg_name(instr.r3.rd), reg_name(instr.r3.rk), reg_name(instr.r3.rj));
	}

	static int AMADD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "amadd%s.d %s, %s, %s", get_atomic_suffix(instr.whole),
			reg_name(instr.r3.rd), reg_name(instr.r3.rk), reg_name(instr.r3.rj));
	}

	static int AMAND_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "amand%s.w %s, %s, %s", get_atomic_suffix(instr.whole),
			reg_name(instr.r3.rd), reg_name(instr.r3.rk), reg_name(instr.r3.rj));
	}

	static int AMAND_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "amand%s.d %s, %s, %s", get_atomic_suffix(instr.whole),
			reg_name(instr.r3.rd), reg_name(instr.r3.rk), reg_name(instr.r3.rj));
	}

	static int AMOR_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "amor%s.w %s, %s, %s", get_atomic_suffix(instr.whole),
			reg_name(instr.r3.rd), reg_name(instr.r3.rk), reg_name(instr.r3.rj));
	}

	static int AMOR_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "amor%s.d %s, %s, %s", get_atomic_suffix(instr.whole),
			reg_name(instr.r3.rd), reg_name(instr.r3.rk), reg_name(instr.r3.rj));
	}

	static int AMXOR_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "amxor%s.w %s, %s, %s", get_atomic_suffix(instr.whole),
			reg_name(instr.r3.rd), reg_name(instr.r3.rk), reg_name(instr.r3.rj));
	}

	static int AMXOR_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "amxor%s.d %s, %s, %s", get_atomic_suffix(instr.whole),
			reg_name(instr.r3.rd), reg_name(instr.r3.rk), reg_name(instr.r3.rj));
	}
};

} // namespace loongarch
