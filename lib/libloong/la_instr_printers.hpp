#pragma once
#include "cpu.hpp"
#include "la_instr.hpp"
#include <cstdio>

namespace loongarch {
// Register names for printing
// r0-r3: $zero, $ra, $tp, $sp
// r4-r11: $a0-$a7
// r12-r20: $t0-$t8
// r21: $r21 (reserved)
// r22: $fp (or $s9)
// r23-r31: $s0-$s8
static const char* reg_names[] = {
	"$zero", "$ra", "$tp", "$sp", "$a0", "$a1", "$a2", "$a3",
	"$a4", "$a5", "$a6", "$a7", "$t0", "$t1", "$t2", "$t3",
	"$t4", "$t5", "$t6", "$t7", "$t8", "$r21", "$fp", "$s0",
	"$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$s8"
};

static inline const char* reg_name(uint32_t r) {
	return r < 32 ? reg_names[r] : "?";
}

static inline const char* cond_names(uint32_t cond) {
	switch (cond) {
	case 0x0:  return "caf";  // Always False (quiet)
	case 0x1:  return "saf";  // Always False (signaling)
	case 0x2:  return "clt";  // Less Than (quiet)
	case 0x3:  return "slt";  // Less Than (signaling)
	case 0x4:  return "ceq";  // Equal (quiet)
	case 0x5:  return "seq";  // Equal (signaling)
	case 0x6:  return "cle";  // Less or Equal (quiet)
	case 0x7:  return "sle";  // Less or Equal (signaling)
	case 0x8:  return "cun";  // Unordered (quiet)
	case 0x9:  return "sun";  // Unordered (signaling)
	case 0xA:  return "cult"; // Unordered or Less Than (quiet)
	case 0xB:  return "sult"; // Unordered or Less Than (signaling)
	case 0xC:  return "cueq"; // Unordered or Equal (quiet)
	case 0xD:  return "sueq"; // Unordered or Equal (signaling)
	case 0xE:  return "cule"; // Unordered or Less or Equal (quiet)
	case 0xF:  return "sule"; // Unordered or Less or Equal (signaling)
	case 0x10: return "cne";  // Not Equal (quiet)
	case 0x11: return "sne";  // Not Equal (signaling)
	case 0x14: return "cor";  // Ordered (quiet)
	case 0x15: return "sor";  // Ordered (signaling)
	case 0x18: return "cune"; // Unordered or Not Equal (quiet)
	case 0x19: return "sune"; // Unordered or Not Equal (signaling)
	default:   return "unknown";
	}
}

struct InstrPrinters {
	using cpu_t = CPU;
	using addr_t = address_t;

	static int INVALID(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "ILLEGAL 0x%08x", instr.whole);
	}

	static int UNIMPLEMENTED(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "UNIMPL 0x%08x", instr.whole);
	}

	static int NOP(char* buf, size_t len, const cpu_t&, la_instruction, addr_t) {
		return snprintf(buf, len, "nop");
	}

	static int RDTIME_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "rdtime.d %s, %s",
			reg_name(instr.r2.rd), reg_name(instr.r2.rj));
	}

	static int CPUCFG(char* buf, size_t len, const cpu_t&, la_instruction, addr_t) {
		return snprintf(buf, len, "cpucfg");
	}

	// === Arithmetic Instructions ===

	static int ADD_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "add.w %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int ADD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "add.d %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int SUB_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "sub.w %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int SUB_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "sub.d %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int ADDI_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		// Check for li.w pseudo-instruction (addi.w rd, $zero, imm)
		if (instr.ri12.rj == 0) {
			return snprintf(buf, len, "li.w %s, %d", reg_name(instr.ri12.rd), imm);
		}
		return snprintf(buf, len, "addi.w %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int ADDI_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		// Check for li.d pseudo-instruction (addi.d rd, $zero, imm)
		if (instr.ri12.rj == 0) {
			return snprintf(buf, len, "li.d %s, %d", reg_name(instr.ri12.rd), imm);
		}
		return snprintf(buf, len, "addi.d %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	// === Division/Modulo Instructions ===

	static int DIV_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "div.w %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int MOD_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "mod.w %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int DIV_WU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "div.wu %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int MOD_WU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "mod.wu %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int DIV_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "div.d %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int MOD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "mod.d %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int DIV_DU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "div.du %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int MOD_DU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "mod.du %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	// === Logical Instructions ===

		static int AND(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
			return snprintf(buf, len, "and %s, %s, %s",
				reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
		}

		static int OR(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
			// Check for move pseudo-instruction (or rd, rj, $zero)
			if (instr.r3.rk == 0) {
				return snprintf(buf, len, "move %s, %s",
					reg_name(instr.r3.rd), reg_name(instr.r3.rj));
			}
			return snprintf(buf, len, "or %s, %s, %s",
				reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
		}

		static int XOR(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
			return snprintf(buf, len, "xor %s, %s, %s",
				reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
		}

	static int NOR(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "nor %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int ORN(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "orn %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int ANDN(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "andn %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int MASKEQZ(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "maskeqz %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int MASKNEZ(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "masknez %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int SLT(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "slt %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int SLTU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "sltu %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int ANDI(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		if (instr.ri12.rd == 0) {
			return snprintf(buf, len, "nop");
		}
		return snprintf(buf, len, "andi %s, %s, 0x%x",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), instr.ri12.imm);
	}

	static int ORI(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
			if (instr.ri12.rj == 0) {
				return snprintf(buf, len, "li.w %s, 0x%x",
					reg_name(instr.ri12.rd), instr.ri12.imm);
			}
			return snprintf(buf, len, "ori %s, %s, 0x%x",
				reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), instr.ri12.imm);
		}

	static int XORI(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "xori %s, %s, 0x%x",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), instr.ri12.imm);
	}

	// === Byte Manipulation ===

	static int BYTEPICK_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t sa2 = (instr.whole >> 15) & 0x3;
		return snprintf(buf, len, "bytepick.d %s, %s, %s, %u",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk), sa2);
	}

	// === Shift Instructions ===

	static int SLL_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
			return snprintf(buf, len, "sll.w %s, %s, %s",
				reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
		}

		static int SRL_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
			return snprintf(buf, len, "srl.w %s, %s, %s",
				reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
		}

		static int SRA_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
			return snprintf(buf, len, "sra.w %s, %s, %s",
				reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
		}

	static int SLL_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "sll.d %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	// Shift immediate printers
	static int SLLI_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t ui5 = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "slli.w %s, %s, 0x%x",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), ui5);
	}

	static int SLLI_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t ui6 = (instr.whole >> 10) & 0x3F;
		return snprintf(buf, len, "slli.d %s, %s, 0x%x",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), ui6);
	}

	static int SRLI_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t ui5 = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "srli.w %s, %s, 0x%x",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), ui5);
	}

	static int SRLI_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t ui6 = (instr.whole >> 10) & 0x3F;
		return snprintf(buf, len, "srli.d %s, %s, 0x%x",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), ui6);
	}

	static int SRAI_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t ui5 = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "srai.w %s, %s, 0x%x",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), ui5);
	}

	static int SRAI_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t ui6 = (instr.whole >> 10) & 0x3F;
		return snprintf(buf, len, "srai.d %s, %s, 0x%x",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), ui6);
	}

	static int ROTRI_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t ui6 = (instr.whole >> 10) & 0x3F;
		return snprintf(buf, len, "rotri.d %s, %s, 0x%x",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), ui6);
	}

	static int SRL_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "srl.d %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}


static int SRA_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
	return snprintf(buf, len, "sra.d %s, %s, %s",
		reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
}	static int ALSL_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		// ALSL.D shift amount is sa2 + 1 (encoded as 0-3 for shift amounts 1-4)
		return snprintf(buf, len, "alsl.d %s, %s, %s, 0x%x",
			reg_name(instr.r3sa2.rd), reg_name(instr.r3sa2.rj),
			reg_name(instr.r3sa2.rk), instr.r3sa2.sa2 + 1);
	}

	// === Load/Store Instructions ===

	static int LD_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "ld.b %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int LD_H(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "ld.h %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int LD_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "ld.w %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int LD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "ld.d %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int LD_BU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "ld.bu %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int LD_HU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "ld.hu %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int LD_WU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "ld.wu %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int PRELD(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "preld %u, %s, %d",
			instr.ri12.rd, reg_name(instr.ri12.rj), imm);
	}

	static int ST_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "st.b %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int ST_H(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "st.h %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int ST_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "st.w %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int ST_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "st.d %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int LDPTR_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_14(instr.ri14.imm) << 2;
		return snprintf(buf, len, "ldptr.w %s, %s, %d",
			reg_name(instr.ri14.rd), reg_name(instr.ri14.rj), imm);
	}

	static int LDPTR_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_14(instr.ri14.imm) << 2;
		return snprintf(buf, len, "ldptr.d %s, %s, %d",
			reg_name(instr.ri14.rd), reg_name(instr.ri14.rj), imm);
	}

	static int STPTR_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_14(instr.ri14.imm) << 2;
		return snprintf(buf, len, "stptr.w %s, %s, %d",
			reg_name(instr.ri14.rd), reg_name(instr.ri14.rj), imm);
	}

	static int STPTR_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_14(instr.ri14.imm) << 2;
		return snprintf(buf, len, "stptr.d %s, %s, %d",
			reg_name(instr.ri14.rd), reg_name(instr.ri14.rj), imm);
	}

	// === Floating-point Load/Store Instructions ===

	static int FLD_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "fld.s $f%d, %s, %d",
			instr.ri12.rd, reg_name(instr.ri12.rj), imm);
	}

	static int FST_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "fst.s $f%d, %s, %d",
			instr.ri12.rd, reg_name(instr.ri12.rj), imm);
	}

	static int FLD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "fld.d $f%d, %s, %d",
			instr.ri12.rd, reg_name(instr.ri12.rj), imm);
	}

	static int FST_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "fst.d $f%d, %s, %d",
			instr.ri12.rd, reg_name(instr.ri12.rj), imm);
	}

	// === Indexed Load/Store Instructions ===

	static int STX_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "stx.b %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int STX_H(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "stx.h %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int STX_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "stx.w %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int STX_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "stx.d %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int FLDX_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "fldx.d $fa%u, %s, %s",
			instr.r3.rd, reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int FLDX_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "fldx.s $fa%u, %s, %s",
			instr.r3.rd, reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int FSTX_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "fstx.d $fa%u, %s, %s",
			instr.r3.rd, reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int FSTX_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "fstx.s $fa%u, %s, %s",
			instr.r3.rd, reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int VLDX(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "vldx $vr%u, %s, %s",
			instr.r3.rd, reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int VSTX(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "vstx $vr%u, %s, %s",
			instr.r3.rd, reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	// === Branch Instructions ===

	static int BEQZ(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t pc) {
		// BEQZ uses ri21 format: rj at bits[9:5], 21-bit offset split across bits[25:10] and [4:0]
		int32_t offset = InstructionHelpers::sign_extend_21(instr.ri21.offs_lo, instr.ri21.offs_hi) << 2;
		addr_t target = pc + offset;
		return snprintf(buf, len, "beqz %s, %d # 0x%lx",
			reg_name(instr.ri21.rj), offset, (unsigned long)target);
	}

	static int BNEZ(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t pc) {
		// BNEZ uses ri21 format: rj at bits[9:5], 21-bit offset split across bits[25:10] and [4:0]
		int32_t offset = InstructionHelpers::sign_extend_21(instr.ri21.offs_lo, instr.ri21.offs_hi) << 2;
		addr_t target = pc + offset;
		return snprintf(buf, len, "bnez %s, %d # 0x%lx",
			reg_name(instr.ri21.rj), offset, (unsigned long)target);
	}

	static int BEQ(char* buf, size_t len, const cpu_t& cpu, la_instruction instr, addr_t pc) {
		int32_t offset = InstructionHelpers::sign_extend_16(instr.ri16.imm) << 2;
		addr_t target = pc + offset;
		return snprintf(buf, len, "beq %s, %s, %d # 0x%lx",
			reg_name(instr.ri16.rj), reg_name(instr.ri16.rd), offset, (unsigned long)target);
	}

	static int BNE(char* buf, size_t len, const cpu_t& cpu, la_instruction instr, addr_t pc) {
		int32_t offset = InstructionHelpers::sign_extend_16(instr.ri16.imm) << 2;
		addr_t target = pc + offset;
		return snprintf(buf, len, "bne %s, %s, %d # 0x%lx",
			reg_name(instr.ri16.rj), reg_name(instr.ri16.rd), offset, (unsigned long)target);
	}

	static int BLT(char* buf, size_t len, const cpu_t& cpu, la_instruction instr, addr_t pc) {
		int32_t offset = InstructionHelpers::sign_extend_16(instr.ri16.imm) << 2;
		addr_t target = pc + offset;
		if (instr.ri16.rd == 0) {
			// BLT rd, $zero, target is equivalent to BLTZ rd, target
			return snprintf(buf, len, "bltz %s, %d # 0x%lx",
				reg_name(instr.ri16.rj), offset, (unsigned long)target);
		} else if (instr.ri16.rj == 0) {
			// BLT $zero, rd, target is equivalent to BGTZ rd, target
			return snprintf(buf, len, "bgtz %s, %d # 0x%lx",
				reg_name(instr.ri16.rd), offset, (unsigned long)target);
		}
		return snprintf(buf, len, "blt %s, %s, %d # 0x%lx",
			reg_name(instr.ri16.rj), reg_name(instr.ri16.rd), offset, (unsigned long)target);
	}

	static int BGE(char* buf, size_t len, const cpu_t& cpu, la_instruction instr, addr_t pc) {
		int32_t offset = InstructionHelpers::sign_extend_16(instr.ri16.imm) << 2;
		addr_t target = pc + offset;
		if (instr.ri16.rd == 0) {
			// BGE rd, $zero, target is equivalent to BGEZ rd, target
			return snprintf(buf, len, "bgez %s, %d # 0x%lx",
				reg_name(instr.ri16.rj), offset, (unsigned long)target);
		} else if (instr.ri16.rj == 0) {
			// BGE $zero, rd, target is equivalent to BLEZ rd, target
			return snprintf(buf, len, "blez %s, %d # 0x%lx",
				reg_name(instr.ri16.rd), offset, (unsigned long)target);
		}
		return snprintf(buf, len, "bge %s, %s, %d # 0x%lx",
			reg_name(instr.ri16.rj), reg_name(instr.ri16.rd), offset, (unsigned long)target);
	}

	static int BLTU(char* buf, size_t len, const cpu_t& cpu, la_instruction instr, addr_t pc) {
		int32_t offset = InstructionHelpers::sign_extend_16(instr.ri16.imm) << 2;
		addr_t target = pc + offset;
		return snprintf(buf, len, "bltu %s, %s, %d # 0x%lx",
			reg_name(instr.ri16.rj), reg_name(instr.ri16.rd), offset, (unsigned long)target);
	}

	static int BGEU(char* buf, size_t len, const cpu_t& cpu, la_instruction instr, addr_t pc) {
		int32_t offset = InstructionHelpers::sign_extend_16(instr.ri16.imm) << 2;
		addr_t target = pc + offset;
		return snprintf(buf, len, "bgeu %s, %s, %d # 0x%lx",
			reg_name(instr.ri16.rj), reg_name(instr.ri16.rd), offset, (unsigned long)target);
	}

	static int B(char* buf, size_t len, const cpu_t& cpu, la_instruction instr, addr_t pc) {
		int32_t offset = InstructionHelpers::sign_extend_26(instr.i26.offs()) << 2;
		addr_t target = pc + offset;
		return snprintf(buf, len, "b  %d # 0x%lx", offset, (unsigned long)target);
	}

	static int BL(char* buf, size_t len, const cpu_t& cpu, la_instruction instr, addr_t pc) {
		int32_t offset = InstructionHelpers::sign_extend_26(instr.i26.offs()) << 2;
		addr_t target = pc + offset;
		return snprintf(buf, len, "bl  %d # 0x%lx", offset, (unsigned long)target);
	}

	static int JIRL(char* buf, size_t len, const cpu_t& cpu, la_instruction instr, addr_t pc) {
		int32_t offset = InstructionHelpers::sign_extend_16(instr.ri16.imm) << 2;
		// Check for ret pseudo-instruction (jirl $zero, $ra, 0)
		if (instr.ri16.rd == 0 && instr.ri16.rj == REG_RA && offset == 0) {
			return snprintf(buf, len, "ret");
		}
		// Check for jr pseudo-instruction (jirl $zero, rj, 0)
		if (instr.ri16.rd == 0 && offset == 0) {
			return snprintf(buf, len, "jr %s", reg_name(instr.ri16.rj));
		}
		return snprintf(buf, len, "jirl %s, %s, %d",
			reg_name(instr.ri16.rd), reg_name(instr.ri16.rj), offset);
	}

	// === Upper Immediate Instructions ===

	static int LU12I_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		const int32_t signed_imm = InstructionHelpers::sign_extend_20(instr.ri20.imm);
		return snprintf(buf, len, "lu12i.w %s, %d",
			reg_name(instr.ri20.rd), signed_imm);
	}

	static int LU32I_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		const int64_t signed_imm = InstructionHelpers::sign_extend_20(instr.ri20.imm);
		return snprintf(buf, len, "lu32i.d %s, %ld",
			reg_name(instr.ri20.rd), signed_imm);
	}

	static int PCADDI(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t pc) {
		int32_t si20 = InstructionHelpers::sign_extend_20(instr.ri20.imm);
		int64_t offset = (int64_t)(si20 << 2);
		addr_t target = pc + offset;
		return snprintf(buf, len, "pcaddi %s, %d  # 0x%lx",
			reg_name(instr.ri20.rd), si20, (unsigned long)target);
	}

	static int PCADDU12I(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t pc) {
		int32_t si20 = InstructionHelpers::sign_extend_20(instr.ri20.imm);
		int64_t offset = (int64_t)(si20 << 12);
		addr_t target = pc + offset;
		return snprintf(buf, len, "pcaddu12i %s, 0x%x  # 0x%lx",
			reg_name(instr.ri20.rd), si20 & 0xFFFFF, (unsigned long)target);
	}

	static int PCALAU12I(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t pc) {
		addr_t pc_aligned = pc & ~((addr_t)0xFFF);
		int64_t offset = (int64_t)(int32_t)(instr.ri20.imm << 12);
		addr_t target = pc_aligned + offset;
		return snprintf(buf, len, "pcalau12i %s, 0x%x  # 0x%lx",
			reg_name(instr.ri20.rd), instr.ri20.imm, (unsigned long)target);
	}

	static int PCADDU18I(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t pc) {
		int32_t si20 = InstructionHelpers::sign_extend_20(instr.ri20.imm);
		int64_t offset = (int64_t)(si20 << 18);
		addr_t target = pc + offset;
		return snprintf(buf, len, "pcaddu18i %s, 0x%x  # 0x%lx",
			reg_name(instr.ri20.rd), si20 & 0xFFFFF, (unsigned long)target);
	}

	static int LU52I_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		const int16_t signed_imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "lu52i.d %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), signed_imm);
	}

	// === Bit Manipulation Instructions ===

	static int BSTRINS_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t msbd = (instr.whole >> 16) & 0x3F;
		uint32_t lsbd = (instr.whole >> 10) & 0x3F;
		return snprintf(buf, len, "bstrins.d %s, %s, 0x%x, 0x%x",
			reg_name(instr.ri16.rd), reg_name(instr.ri16.rj), msbd, lsbd);
	}

	static int BSTRPICK_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t msbd = (instr.whole >> 16) & 0x3F;
		uint32_t lsbd = (instr.whole >> 10) & 0x3F;
		return snprintf(buf, len, "bstrpick.d %s, %s, 0x%x, 0x%x",
			reg_name(instr.ri16.rd), reg_name(instr.ri16.rj), msbd, lsbd);
	}

	static int BSTRINS_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t msbw = (instr.whole >> 16) & 0x1F;
		uint32_t lsbw = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "bstrins.w %s, %s, 0x%x, 0x%x",
			reg_name(instr.ri16.rd), reg_name(instr.ri16.rj), msbw, lsbw);
	}

	static int BSTRPICK_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t msbw = (instr.whole >> 16) & 0x1F;
		uint32_t lsbw = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "bstrpick.w %s, %s, 0x%x, 0x%x",
			reg_name(instr.ri16.rd), reg_name(instr.ri16.rj), msbw, lsbw);
	}

	// === System Instructions ===

	static int SYSCALL(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		const uint32_t imm = instr.whole & 0xFFF;
		return snprintf(buf, len, "syscall 0x%x", imm);
	}

	// === Memory Barriers ===

	static int DBAR(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t hint = instr.whole & 0x7FFF;
		return snprintf(buf, len, "dbar 0x%x", hint);
	}

	static int IBAR(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t hint = instr.whole & 0x7FFF;
		return snprintf(buf, len, "ibar 0x%x", hint);
	}

	// === LL/SC Atomics ===

	static int LL_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_14(instr.ri14.imm) << 2;
		return snprintf(buf, len, "ll.w %s, %s, %d",
			reg_name(instr.ri14.rd), reg_name(instr.ri14.rj), imm);
	}

	static int LL_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_14(instr.ri14.imm) << 2;
		return snprintf(buf, len, "ll.d %s, %s, %d",
			reg_name(instr.ri14.rd), reg_name(instr.ri14.rj), imm);
	}

	static int SC_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_14(instr.ri14.imm) << 2;
		return snprintf(buf, len, "sc.w %s, %s, %d",
			reg_name(instr.ri14.rd), reg_name(instr.ri14.rj), imm);
	}

	static int SC_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_14(instr.ri14.imm) << 2;
		return snprintf(buf, len, "sc.d %s, %s, %d",
			reg_name(instr.ri14.rd), reg_name(instr.ri14.rj), imm);
	}

	// === Indexed Load Instructions ===

	static int LDX_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "ldx.b %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int LDX_H(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "ldx.h %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int LDX_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "ldx.w %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int LDX_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "ldx.d %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int LDX_BU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "ldx.bu %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int LDX_HU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "ldx.hu %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int LDX_WU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "ldx.wu %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	// === Multiply Instructions ===

	static int MUL_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "mul.w %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int MULH_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "mulh.w %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int MULH_WU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "mulh.wu %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int MUL_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "mul.d %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int MULH_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "mulh.d %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int MULH_DU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "mulh.du %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	// === Comparison Immediate Instructions ===

	static int SLTI(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "slti %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int SLTUI(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "sltui %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	// === Rotate Instructions ===

	static int ROTR_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "rotr.w %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int ROTR_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "rotr.d %s, %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int ROTRI_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t ui5 = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "rotri.w %s, %s, 0x%x",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj), ui5);
	}

	// === Bit Manipulation 2R Instructions ===

	static int EXT_W_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "ext.w.b %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj));
	}

	static int EXT_W_H(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "ext.w.h %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj));
	}

	static int CLO_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "clo.w %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj));
	}

	static int CLZ_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "clz.w %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj));
	}

	static int CTO_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "cto.w %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj));
	}

	static int CTZ_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "ctz.w %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj));
	}

	static int CLO_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "clo.d %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj));
	}

	static int CLZ_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "clz.d %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj));
	}

	static int CTO_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "cto.d %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj));
	}

	static int CTZ_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "ctz.d %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj));
	}

	static int REVB_2H(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "revb.2h %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj));
	}

	static int REVB_4H(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "revb.4h %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj));
	}

	static int REVB_2W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "revb.2w %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj));
	}

	static int REVB_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "revb.d %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj));
	}

	static int REVH_2W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "revh.2w %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj));
	}

	static int REVH_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "revh.d %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj));
	}

	static int BITREV_4B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "bitrev.4b %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj));
	}

	static int BITREV_8B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "bitrev.8b %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj));
	}

	static int BITREV_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "bitrev.w %s, %s",
			reg_name(instr.r2.rd), reg_name(instr.r2.rj));
	}

	static int BITREV_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "bitrev.d %s, %s",
			reg_name(instr.r2.rd), reg_name(instr.r2.rj));
	}

	// === ALSL.W ===

	static int ALSL_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "alsl.w %s, %s, %s, %u",
			reg_name(instr.r3sa2.rd), reg_name(instr.r3sa2.rj),
			reg_name(instr.r3sa2.rk), instr.r3sa2.sa2);
	}

	// === LSX Vector Load/Store ===

	static int VLD(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "vld $vr%u, %s, %d",
			instr.ri12.rd, reg_name(instr.ri12.rj), imm);
	}

	static int VST(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "vst $vr%u, %s, %d",
			instr.ri12.rd, reg_name(instr.ri12.rj), imm);
	}

	static int XVLD(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "xvld $xr%u, %s, %d",
			instr.ri12.rd, reg_name(instr.ri12.rj), imm);
	}

	static int XVST(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "xvst $xr%u, %s, %d",
			instr.ri12.rd, reg_name(instr.ri12.rj), imm);
	}

	// === LSX Vector Test Instructions ===

	static int VSETANYEQZ_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t cd = instr.whole & 0x7;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "vsetanyeqz.b $fcc%u, $vr%u", cd, vj);
	}

	static int VSETALLNEZ_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t cd = instr.whole & 0x7;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "vsetallnez.b $fcc%u, $vr%u", cd, vj);
	}

	static int VMSKNZ_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "vmsknz.b $vr%u, $vr%u", vd, vj);
	}

	static int VSEQI(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		static constexpr char sizes[] = {'b', 'h', 'w', 'd'};
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		int32_t si5 = (instr.whole >> 10) & 0x1F;
		if (si5 & 0x10) si5 |= 0xFFFFFFE0;
		// Extract size from opcode: bits[16:15] encode the size (0=b, 1=h, 2=w, 3=d)
		uint32_t size_idx = (instr.whole >> 15) & 0x3;
		return snprintf(buf, len, "vseqi.%c $vr%u, $vr%u, %d", sizes[size_idx], vd, vj, si5);
	}

	static int MOVFR2GR_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t rd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "movfr2gr.s %s, $fa%u", reg_name(rd), fj);
	}

	static int MOVFR2GR_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t rd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "movfr2gr.d %s, $fa%u", reg_name(rd), fj);
	}

	static int MOVGR2FR_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.whole & 0x1F;
		uint32_t rj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "movgr2fr.w $fa%u, %s", fd, reg_name(rj));
	}

	static int MOVGR2FR_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.whole & 0x1F;
		uint32_t rj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "movgr2fr.d $fa%u, %s", fd, reg_name(rj));
	}

	static int MOVFCSR2GR(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t rd = instr.whole & 0x1F;
		uint32_t fcsr_idx = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "movfcsr2gr %s, $fcsr%u", reg_name(rd), fcsr_idx);
	}

	static int MOVFR2CF(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t cd = instr.whole & 0x7;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "movfr2cf $fcc%u, $fa%u", cd, fj);
	}

	static int MOVCF2FR(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.whole & 0x1F;
		uint32_t cj = (instr.whole >> 5) & 0x7;
		return snprintf(buf, len, "movcf2fr $fa%u, $fcc%u", fd, cj);
	}

	static int MOVGR2CF(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t cd = instr.whole & 0x7;
		uint32_t rj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "movgr2cf $fcc%u, %s", cd, reg_name(rj));
	}

	static int MOVCF2GR(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t rd = instr.whole & 0x1F;
		uint32_t cj = (instr.whole >> 5) & 0x7;
		return snprintf(buf, len, "movcf2gr %s, $fcc%u", reg_name(rd), cj);
	}

	static int VFCMP_COND_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		uint32_t cond = (instr.whole >> 15) & 0x1F;
		const char* mnemonic = cond_names(cond);
		return snprintf(buf, len, "vfcmp.%s.d $vr%u, $vr%u, $vr%u", mnemonic, vd, vj, vk);
	}

	static int FCMP_COND_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t cd = instr.whole & 0x7;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		uint32_t fk = (instr.whole >> 10) & 0x1F;
		const uint32_t cond = (instr.whole >> 15) & 0x1F;
		const char* mnemonic = cond_names(cond);
		return snprintf(buf, len, "fcmp.%s.s $fcc%u, $fa%u, $fa%u", mnemonic, cd, fj, fk);
	}

	static int FCMP_COND_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t cd = instr.whole & 0x7;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		uint32_t fk = (instr.whole >> 10) & 0x1F;
		const uint32_t cond = (instr.whole >> 15) & 0x1F;
		const char* mnemonic = cond_names(cond);
		return snprintf(buf, len, "fcmp.%s.d $fcc%u, $fa%u, $fa%u", mnemonic, cd, fj, fk);
	}

	static int FSEL(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		uint32_t fk = (instr.whole >> 10) & 0x1F;
		uint32_t ca = (instr.whole >> 15) & 0x7;
		return snprintf(buf, len, "fsel $fa%u, $fa%u, $fa%u, $fcc%u", fd, fj, fk, ca);
	}

	static int FABS_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "fabs.d $fa%u, $fa%u", fd, fj);
	}

	static int FABS_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "fabs.s $fa%u, $fa%u", fd, fj);
	}

	static int FNEG_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "fneg.d $fa%u, $fa%u", fd, fj);
	}

	static int FMOV_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "fmov.d $fa%u, $fa%u", fd, fj);
	}

	static int FMOV_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "fmov.s $fa%u, $fa%u", fd, fj);
	}

	static int FCLASS(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		static constexpr char sizes[] = {'d', 's'};
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		// Extract size from opcode: bit[10] (0=d, 1=s)
		uint32_t size_idx = (instr.whole >> 10) & 0x1;
		return snprintf(buf, len, "fclass.%c $fa%u, $fa%u", sizes[size_idx], fd, fj);
	}

	static int FFINT_D_L(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "ffint.d.l $fa%u, $fa%u", fd, fj);
	}

	static int FFINT_D_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "ffint.d.w $fa%u, $fa%u", fd, fj);
	}

	static int FFINT_S_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "ffint.s.w $fa%u, $fa%u", fd, fj);
	}

	static int FFINT_S_L(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "ffint.s.l $fa%u, $fa%u", fd, fj);
	}

	static int FCVT_S_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "fcvt.s.d $fa%u, $fa%u", fd, fj);
	}

	static int FCVT_D_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "fcvt.d.s $fa%u, $fa%u", fd, fj);
	}

	static int FTINTRZ_W_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "ftintrz.w.s $fa%u, $fa%u", fd, fj);
	}

	static int FTINTRZ_W_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "ftintrz.w.d $fa%u, $fa%u", fd, fj);
	}

	static int FTINTRZ_L_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "ftintrz.l.s $fa%u, $fa%u", fd, fj);
	}

	static int FTINTRZ_L_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "ftintrz.l.d $fa%u, $fa%u", fd, fj);
	}

	static int FADD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;
		return snprintf(buf, len, "fadd.d $fa%u, $fa%u, $fa%u", fd, fj, fk);
	}

	static int FADD_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;
		return snprintf(buf, len, "fadd.s $fa%u, $fa%u, $fa%u", fd, fj, fk);
	}

	static int FMUL_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;
		return snprintf(buf, len, "fmul.d $fa%u, $fa%u, $fa%u", fd, fj, fk);
	}

	static int FMUL_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;
		return snprintf(buf, len, "fmul.s $fa%u, $fa%u, $fa%u", fd, fj, fk);
	}

	static int FSUB_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;
		return snprintf(buf, len, "fsub.d $fa%u, $fa%u, $fa%u", fd, fj, fk);
	}

	static int FSUB_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;
		return snprintf(buf, len, "fsub.s $fa%u, $fa%u, $fa%u", fd, fj, fk);
	}

	static int FDIV_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;
		return snprintf(buf, len, "fdiv.d $fa%u, $fa%u, $fa%u", fd, fj, fk);
	}

	static int FDIV_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;
		return snprintf(buf, len, "fdiv.s $fa%u, $fa%u, $fa%u", fd, fj, fk);
	}

	static int FMAX_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;
		return snprintf(buf, len, "fmax.s $fa%u, $fa%u, $fa%u", fd, fj, fk);
	}

	static int FMIN_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;
		return snprintf(buf, len, "fmin.s $fa%u, $fa%u, $fa%u", fd, fj, fk);
	}

	static int FMAX_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;
		return snprintf(buf, len, "fmax.d $fa%u, $fa%u, $fa%u", fd, fj, fk);
	}

	static int FMIN_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;
		return snprintf(buf, len, "fmin.d $fa%u, $fa%u, $fa%u", fd, fj, fk);
	}

	static int FMSUB_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r4.rd;
		uint32_t fj = instr.r4.rj;
		uint32_t fk = instr.r4.rk;
		uint32_t fa = instr.r4.ra;
		return snprintf(buf, len, "fmsub.d $fa%u, $fa%u, $fa%u, $fa%u", fd, fj, fk, fa);
	}

	static int FMADD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r4.rd;
		uint32_t fj = instr.r4.rj;
		uint32_t fk = instr.r4.rk;
		uint32_t fa = instr.r4.ra;
		return snprintf(buf, len, "fmadd.d $fa%u, $fa%u, $fa%u, $fa%u", fd, fj, fk, fa);
	}

	static int FMADD_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r4.rd;
		uint32_t fj = instr.r4.rj;
		uint32_t fk = instr.r4.rk;
		uint32_t fa = instr.r4.ra;
		return snprintf(buf, len, "fmadd.s $fa%u, $fa%u, $fa%u, $fa%u", fd, fj, fk, fa);
	}

	static int FMSUB_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r4.rd;
		uint32_t fj = instr.r4.rj;
		uint32_t fk = instr.r4.rk;
		uint32_t fa = instr.r4.ra;
		return snprintf(buf, len, "fmsub.s $fa%u, $fa%u, $fa%u, $fa%u", fd, fj, fk, fa);
	}

	static int FNMADD_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r4.rd;
		uint32_t fj = instr.r4.rj;
		uint32_t fk = instr.r4.rk;
		uint32_t fa = instr.r4.ra;
		return snprintf(buf, len, "fnmadd.s $fa%u, $fa%u, $fa%u, $fa%u", fd, fj, fk, fa);
	}

	static int FNMADD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r4.rd;
		uint32_t fj = instr.r4.rj;
		uint32_t fk = instr.r4.rk;
		uint32_t fa = instr.r4.ra;
		return snprintf(buf, len, "fnmadd.d $fa%u, $fa%u, $fa%u, $fa%u", fd, fj, fk, fa);
	}

	static int FNMSUB_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r4.rd;
		uint32_t fj = instr.r4.rj;
		uint32_t fk = instr.r4.rk;
		uint32_t fa = instr.r4.ra;
		return snprintf(buf, len, "fnmsub.s $fa%u, $fa%u, $fa%u, $fa%u", fd, fj, fk, fa);
	}

	static int FNMSUB_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r4.rd;
		uint32_t fj = instr.r4.rj;
		uint32_t fk = instr.r4.rk;
		uint32_t fa = instr.r4.ra;
		return snprintf(buf, len, "fnmsub.d $fa%u, $fa%u, $fa%u, $fa%u", fd, fj, fk, fa);
	}

	static int VFRSTPI_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t ui5 = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vfrstpi.b $vr%u, $vr%u, 0x%x", vd, vj, ui5);
	}

	// === LSX Condition Branches ===

	static int BCNEZ(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t pc) {
		uint32_t cj = (instr.whole >> 5) & 0x7;
		int32_t offs = ((instr.whole >> 10) & 0xFFFF) | ((instr.whole & 0x1F) << 16);
		if (offs & 0x100000) offs |= 0xFFE00000;  // Sign extend 21-bit
		offs <<= 2;
		addr_t target = pc + offs;
		return snprintf(buf, len, "bcnez $fcc%u, 0x%lx", cj, (unsigned long)target);
	}

	static int BCEQZ(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t pc) {
		uint32_t cj = (instr.whole >> 5) & 0x7;
		int32_t offs = ((instr.whole >> 10) & 0xFFFF) | ((instr.whole & 0x1F) << 16);
		if (offs & 0x100000) offs |= 0xFFE00000;  // Sign extend 21-bit
		offs <<= 2;
		addr_t target = pc + offs;
		return snprintf(buf, len, "bceqz $fcc%u, 0x%lx", cj, (unsigned long)target);
	}

	// === LSX Vector Element Extraction ===

	static int VPICKVE2GR(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t rd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t top16 = (instr.whole >> 16) & 0xFFFF;
		uint32_t subop = (instr.whole >> 12) & 0xF;

		// Determine size suffix and immediate width
		const char* size_suffix;
		uint32_t imm_mask;
		switch (subop) {
			case 0x8: size_suffix = (top16 == 0x72EF) ? ".b" : ".bu"; imm_mask = 0xF; break;
			case 0xC: size_suffix = (top16 == 0x72EF) ? ".h" : ".hu"; imm_mask = 0x7; break;
			case 0xE: size_suffix = (top16 == 0x72EF) ? ".w" : ".wu"; imm_mask = 0x3; break;
			case 0xF: size_suffix = (top16 == 0x72EF) ? ".d" : ".du"; imm_mask = 0x1; break;
			default: size_suffix = ".?"; imm_mask = 0xF; break;
		}

		uint32_t imm = (instr.whole >> 10) & imm_mask;
		return snprintf(buf, len, "vpickve2gr%s %s, $vr%u, 0x%x",
			size_suffix, reg_name(rd), vj, imm);
	}

	// === LSX Vector Arithmetic/Logic ===

	static int VSUB(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		static constexpr char sizes[] = {'b', 'h', 'w', 'd'};
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		// Extract size from opcode: bits[16:15] encode the size (0=b, 1=h, 2=w, 3=d)
		uint32_t size_idx = (instr.whole >> 15) & 0x3;
		return snprintf(buf, len, "vsub.%c $vr%u, $vr%u, $vr%u", sizes[size_idx], vd, vj, vk);
	}

	static int VMUL(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		static constexpr char sizes[] = {'b', 'h', 'w', 'd'};
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		// Extract size from opcode: bits[16:15] encode the size (0=b, 1=h, 2=w, 3=d)
		uint32_t size_idx = (instr.whole >> 15) & 0x3;
		return snprintf(buf, len, "vmul.%c $vr%u, $vr%u, $vr%u", sizes[size_idx], vd, vj, vk);
	}

	static int VMADD(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		static constexpr char sizes[] = {'b', 'h', 'w', 'd'};
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		// Extract size from opcode: bits[16:15] encode the size (0=b, 1=h, 2=w, 3=d)
		uint32_t size_idx = (instr.whole >> 15) & 0x3;
		return snprintf(buf, len, "vmadd.%c $vr%u, $vr%u, $vr%u", sizes[size_idx], vd, vj, vk);
	}

	static int VADDI(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		static constexpr const char* sizes[] = {"bu", "hu", "wu", "du"};
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t uk5 = (instr.whole >> 10) & 0x1F; // Unsigned 5-bit immediate
		// Extract size from opcode: bits[16:15] encode the size (0=bu, 1=hu, 2=wu, 3=du)
		uint32_t size_idx = (instr.whole >> 15) & 0x3;
		return snprintf(buf, len, "vaddi.%s $vr%u, $vr%u, %u", sizes[size_idx], vd, vj, uk5);
	}

	static int VSEQ_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vseq.b $vr%u, $vr%u, $vr%u", vd, vj, vk);
	}

	static int VSLT(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		static constexpr char sizes[] = {'b', 'h', 'w', 'd'};
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		// Extract size from opcode: bits[16:15] encode the size (0=b, 1=h, 2=w, 3=d)
		uint32_t size_idx = (instr.whole >> 15) & 0x3;
		return snprintf(buf, len, "vslt.%c $vr%u, $vr%u, $vr%u", sizes[size_idx], vd, vj, vk);
	}

	static int VILVL(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		static constexpr char sizes[] = {'b', 'h', 'w', 'd'};
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		// Extract size from opcode: bits[16:15] encode the size (0=b, 1=h, 2=w, 3=d)
		uint32_t size_idx = (instr.whole >> 15) & 0x3;
		return snprintf(buf, len, "vilvl.%c $vr%u, $vr%u, $vr%u", sizes[size_idx], vd, vj, vk);
	}

	static int VILVH_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vilvh.d $vr%u, $vr%u, $vr%u", vd, vj, vk);
	}

	static int VPICKEV_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vpickev.w $vr%u, $vr%u, $vr%u", vd, vj, vk);
	}

	static int VNOR_V(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vnor.v $vr%u, $vr%u, $vr%u", vd, vj, vk);
	}

	static int VORN_V(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vorn.v $vr%u, $vr%u, $vr%u", vd, vj, vk);
	}

	static int VAND_V(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vand.v $vr%u, $vr%u, $vr%u", vd, vj, vk);
	}

	static int VBITREVI_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t imm = (instr.whole >> 10) & 0x3F;
		return snprintf(buf, len, "vbitrevi.d $vr%u, $vr%u, 0x%x", vd, vj, imm);
	}

	static int VSLLI(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		static constexpr char sizes[] = {'b', 'h', 'w', 'd'};
		static constexpr uint32_t imm_masks[] = {0x7, 0xF, 0x1F, 0x3F};  // 3, 4, 5, 6 bits
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;

		// Determine size based on bits[31:15]
		uint32_t bits15 = instr.whole >> 15;
		uint32_t size_idx;
		if (bits15 == 0xE658) {
			// vslli.b and vslli.h both have bits[31:15] = 0xe658, differentiate by bit 14
			uint32_t bit14 = (instr.whole >> 14) & 1;
			size_idx = bit14 ? 1 : 0;  // 0=b, 1=h
		} else if (bits15 == 0xE659) {
			size_idx = 2;  // w
		} else {  // 0xE65A
			size_idx = 3;  // d
		}

		uint32_t imm = (instr.whole >> 10) & imm_masks[size_idx];
		return snprintf(buf, len, "vslli.%c $vr%u, $vr%u, 0x%x", sizes[size_idx], vd, vj, imm);
	}

	static int VPCNT(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		static constexpr char sizes[] = {'b', 'h', 'w', 'd'};
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		// Extract size from opcode: bits[11:10] encode the size (0=b, 1=h, 2=w, 3=d)
		uint32_t size_idx = (instr.whole >> 10) & 0x3;
		return snprintf(buf, len, "vpcnt.%c $vr%u, $vr%u", sizes[size_idx], vd, vj);
	}

	static int VLDI(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t imm13 = (instr.whole >> 5) & 0x1FFF;
		return snprintf(buf, len, "vldi $vr%u, %d", vd, (int)(int16_t)(imm13 << 3) >> 3);
	}

	static int VORI_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t imm8 = (instr.whole >> 10) & 0xFF;
		return snprintf(buf, len, "vori.b $vr%u, $vr%u, 0x%x", vd, vj, imm8);
	}

	static int VFADD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vfadd.d $vr%u, $vr%u, $vr%u", vd, vj, vk);
	}

	static int VFDIV_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vfdiv.d $vr%u, $vr%u, $vr%u", vd, vj, vk);
	}

	static int VFMUL(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		static constexpr char sizes[] = {'s', 'd'};
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		// Extract size from opcode: bit[16] determines s/d (0=s, 1=d)
		uint32_t size_idx = (instr.whole >> 16) & 0x1;
		return snprintf(buf, len, "vfmul.%c $vr%u, $vr%u, $vr%u", sizes[size_idx], vd, vj, vk);
	}

	static int VFTINTRZ_W_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "vftintrz.w.s $vr%u, $vr%u", vd, vj);
	}

	static int VFTINTRZ_L_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "vftintrz.l.d $vr%u, $vr%u", vd, vj);
	}

	static int VHADDW_D_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vhaddw.d.w $vr%u, $vr%u, $vr%u", vd, vj, vk);
	}

	static int XVHADDW_D_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvhaddw.d.w $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVPICKVE2GR_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t rd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t ui3 = (instr.whole >> 10) & 0x7;
		return snprintf(buf, len, "xvpickve2gr.w %s, $xr%u, %u", reg_name(rd), xj, ui3);
	}

	static int XVADD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvadd.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVBITSEL_V(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "xvbitsel.v $xr%u, $xr%u, $xr%u, $xr%u", xd, xj, xk, xa);
	}

	static int XVFCMP_COND_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t cond = (instr.whole >> 15) & 0x1F;
		const char* mnemonic = cond_names(cond);
		return snprintf(buf, len, "xvfcmp.%s.d $xr%u, $xr%u, $xr%u", mnemonic, xd, xj, xk);
	}

	static int XVHADDW_Q_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvhaddw.q.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int VFMADD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		uint32_t va = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "vfmadd.d $vr%u, $vr%u, $vr%u, $vr%u", vd, vj, vk, va);
	}

	static int VFNMADD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		uint32_t va = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "vfnmadd.d $vr%u, $vr%u, $vr%u, $vr%u", vd, vj, vk, va);
	}

	static int VOR_V(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vor.v $vr%u, $vr%u, $vr%u", vd, vj, vk);
	}

	static int VXOR_V(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vxor.v $vr%u, $vr%u, $vr%u", vd, vj, vk);
	}

	static int VREPLVEI_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t idx = (instr.whole >> 10) & 0x1;
		return snprintf(buf, len, "vreplvei.d $vr%u, $vr%u, 0x%x", vd, vj, idx);
	}

	static int VREPLGR2VR(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		static constexpr char sizes[] = {'b', 'h', 'w', 'd'};
		uint32_t vd = instr.whole & 0x1F;
		uint32_t rj = (instr.whole >> 5) & 0x1F;
		// Extract size from subop: bits[11:10] (0=b, 1=h, 2=w, 3=d)
		uint32_t size_idx = (instr.whole >> 10) & 0x3;
		return snprintf(buf, len, "vreplgr2vr.%c $vr%u, %s", sizes[size_idx], vd, reg_name(rj));
	}

	static int VINSGR2VR(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		static constexpr char sizes[] = {'b', 'h', 'w', 'd'};
		static constexpr uint32_t idx_masks[] = {0xF, 0x7, 0x3, 0x1};
		uint32_t vd = instr.whole & 0x1F;
		uint32_t rj = (instr.whole >> 5) & 0x1F;
		// Determine size by checking bit patterns: .b=10, .h=110, .w=1110, .d=11110
		uint32_t bits15_14 = (instr.whole >> 14) & 0x3;
		uint32_t bits15_13 = (instr.whole >> 13) & 0x7;
		uint32_t bits15_12 = (instr.whole >> 12) & 0xF;
		uint32_t size_idx;
		if (bits15_14 == 0x2) size_idx = 0;       // .b: 10
		else if (bits15_13 == 0x6) size_idx = 1;  // .h: 110
		else if (bits15_12 == 0xE) size_idx = 2;  // .w: 1110
		else size_idx = 3;                         // .d: 11110
		uint32_t idx = (instr.whole >> 10) & idx_masks[size_idx];
		return snprintf(buf, len, "vinsgr2vr.%c $vr%u, %s, 0x%x", sizes[size_idx], vd, reg_name(rj), idx);
	}

	static int VADDI_BU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t imm = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vaddi.bu $vr%u, $vr%u, 0x%x", vd, vj, imm);
	}

	static int VADD(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		static constexpr char sizes[] = {'b', 'h', 'w', 'd'};
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		// Extract size from opcode: bits[16:15] encode the size (0=b, 1=h, 2=w, 3=d)
		uint32_t size_idx = (instr.whole >> 15) & 0x3;
		return snprintf(buf, len, "vadd.%c $vr%u, $vr%u, $vr%u", sizes[size_idx], vd, vj, vk);
	}

	static int VSHUF_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		uint32_t va = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "vshuf.b $vr%u, $vr%u, $vr%u, $vr%u", vd, vj, vk, va);
	}

	static int VBITSEL_V(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		uint32_t va = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "vbitsel.v $vr%u, $vr%u, $vr%u, $vr%u", vd, vj, vk, va);
	}

	static int VMAX(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		static constexpr char sizes[] = {'b', 'h', 'w', 'd'};
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		uint32_t bits15 = instr.whole >> 15;

		// Determine size and signedness
		const char* suffix;
		if (bits15 >= 0xE0E0 && bits15 <= 0xE0E3) {
			// Signed: 0xE0E0-E0E3
			char size = sizes[bits15 - 0xE0E0];
			static char buf_suffix[4];
			snprintf(buf_suffix, sizeof(buf_suffix), ".%c", size);
			suffix = buf_suffix;
		} else {
			// Unsigned: 0xE0E8-E0EB
			char size = sizes[bits15 - 0xE0E8];
			static char buf_suffix[4];
			snprintf(buf_suffix, sizeof(buf_suffix), ".%cu", size);
			suffix = buf_suffix;
		}
		return snprintf(buf, len, "vmax%s $vr%u, $vr%u, $vr%u", suffix, vd, vj, vk);
	}

	static int VMIN(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		static constexpr char sizes[] = {'b', 'h', 'w', 'd'};
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		uint32_t bits15 = instr.whole >> 15;

		// Determine size and signedness
		const char* suffix;
		if (bits15 >= 0xE0E4 && bits15 <= 0xE0E7) {
			// Signed: 0xE0E4-E0E7
			char size = sizes[bits15 - 0xE0E4];
			static char buf_suffix[4];
			snprintf(buf_suffix, sizeof(buf_suffix), ".%c", size);
			suffix = buf_suffix;
		} else {
			// Unsigned: 0xE0EC-E0EF
			char size = sizes[bits15 - 0xE0EC];
			static char buf_suffix[4];
			snprintf(buf_suffix, sizeof(buf_suffix), ".%cu", size);
			suffix = buf_suffix;
		}
		return snprintf(buf, len, "vmin%s $vr%u, $vr%u, $vr%u", suffix, vd, vj, vk);
	}

	// === LASX (256-bit) Instruction Printers ===

	static int XVREPLGR2VR_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t rj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "xvreplgr2vr.b $xr%u, %s", xd, reg_name(rj));
	}

	static int XVXOR_V(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvxor.v $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVSUB_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvsub.w $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVMIN_BU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvmin.bu $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVMAX_BU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvmax.bu $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVMSKNZ_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "xvmsknz.b $xr%u, $xr%u", xd, xj);
	}

	static int XVPICKVE_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t idx = (instr.whole >> 10) & 0x7;
		return snprintf(buf, len, "xvpickve.w $xr%u, $xr%u, 0x%x", xd, xj, idx);
	}

	static int XVSETANYEQZ_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t cd = instr.whole & 0x7;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "xvsetanyeqz.b $fcc%u, $xr%u", cd, xj);
	}

	static int XVSEQ_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvseq.b $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVSETEQZ_V(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t cd = instr.whole & 0x7;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "xvseteqz.v $fcc%u, $xr%u", cd, xj);
	}

	static int XVPERMI_Q(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t imm = (instr.whole >> 10) & 0xFF;
		return snprintf(buf, len, "xvpermi.q $xr%u, $xr%u, 0x%x", xd, xj, imm);
	}

	static int XVLDX(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "xvldx $xr%u, %s, %s",
			instr.r3.rd, reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int XVSTX(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "xvstx $xr%u, %s, %s",
			instr.r3.rd, reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int XVFADD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvfadd.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVFMUL_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvfmul.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVFDIV_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvfdiv.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVFSUB_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvfsub.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVBITREVI_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t imm = (instr.whole >> 10) & 0x3F;
		return snprintf(buf, len, "xvbitrevi.d $xr%u, $xr%u, 0x%x", xd, xj, imm);
	}

	static int XVREPLVE_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvreplve.d $xr%u, $xr%u, $r%u", xd, xk, xj);
	}

	static int XVFMADD_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "xvfmadd.s $xr%u, $xr%u, $xr%u, $xr%u", xd, xj, xk, xa);
	}

	static int XVFMADD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "xvfmadd.d $xr%u, $xr%u, $xr%u, $xr%u", xd, xj, xk, xa);
	}

	static int XVFMSUB_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "xvfmsub.s $xr%u, $xr%u, $xr%u, $xr%u", xd, xj, xk, xa);
	}

	static int XVFMSUB_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "xvfmsub.d $xr%u, $xr%u, $xr%u, $xr%u", xd, xj, xk, xa);
	}

	static int XVFNMADD_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "xvfnmadd.s $xr%u, $xr%u, $xr%u, $xr%u", xd, xj, xk, xa);
	}

	static int XVFNMADD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "xvfnmadd.d $xr%u, $xr%u, $xr%u, $xr%u", xd, xj, xk, xa);
	}

	static int XVFNMSUB_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "xvfnmsub.s $xr%u, $xr%u, $xr%u, $xr%u", xd, xj, xk, xa);
	}

	static int XVFNMSUB_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "xvfnmsub.d $xr%u, $xr%u, $xr%u, $xr%u", xd, xj, xk, xa);
	}

	static int XVORI_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t imm8 = (instr.whole >> 10) & 0xFF;
		return snprintf(buf, len, "xvori.b $xr%u, $xr%u, 0x%x", xd, xj, imm8);
	}

	static int XVXORI_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t imm8 = (instr.whole >> 10) & 0xFF;
		return snprintf(buf, len, "xvxori.b $xr%u, $xr%u, 0x%x", xd, xj, imm8);
	}

	static int XVILVL_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvilvl.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVILVH_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvilvh.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVPERMI_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t imm8 = (instr.whole >> 10) & 0xFF;
		return snprintf(buf, len, "xvpermi.d $xr%u, $xr%u, 0x%x", xd, xj, imm8);
	}

	static int XVPACKEV_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvpackev.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVPACKOD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvpackod.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVPICKEV_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvpickev.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVPICKEV_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvpickev.w $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVPICKOD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvpickod.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVLDI(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t imm13 = (instr.whole >> 5) & 0x1FFF;
		return snprintf(buf, len, "xvldi $xr%u, %d", xd, (int)(int16_t)(imm13 << 3) >> 3);
	}

};

} // loongarch
