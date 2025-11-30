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

// Template instruction printers shared between LA32 and LA64
template <int W>
struct InstrPrinters {
	using cpu_t = CPU<W>;
	using addr_t = address_type<W>;

	static int UNIMPLEMENTED(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "UNIMPL 0x%08x", instr.whole);
	}

	static int NOP(char* buf, size_t len, const cpu_t&, la_instruction, addr_t) {
		return snprintf(buf, len, "nop");
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
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		// Check for li.w pseudo-instruction (addi.w rd, $zero, imm)
		if (instr.ri12.rj == 0) {
			return snprintf(buf, len, "li.w %s, %d", reg_name(instr.ri12.rd), imm);
		}
		return snprintf(buf, len, "addi.w %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int ADDI_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
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
		return snprintf(buf, len, "andi %s, %s, 0x%x",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), instr.ri12.imm);
	}

	static int ORI(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
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
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "ld.b %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int LD_H(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "ld.h %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int LD_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "ld.w %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int LD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "ld.d %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int LD_BU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "ld.bu %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int LD_HU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "ld.hu %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int LD_WU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "ld.wu %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int ST_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "st.b %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int ST_H(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "st.h %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int ST_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "st.w %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int ST_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "st.d %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int LDPTR_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_14(instr.ri14.imm) << 2;
		return snprintf(buf, len, "ldptr.w %s, %s, %d",
			reg_name(instr.ri14.rd), reg_name(instr.ri14.rj), imm);
	}

	static int LDPTR_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_14(instr.ri14.imm) << 2;
		return snprintf(buf, len, "ldptr.d %s, %s, %d",
			reg_name(instr.ri14.rd), reg_name(instr.ri14.rj), imm);
	}

	static int STPTR_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_14(instr.ri14.imm) << 2;
		return snprintf(buf, len, "stptr.w %s, %s, %d",
			reg_name(instr.ri14.rd), reg_name(instr.ri14.rj), imm);
	}

	static int STPTR_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_14(instr.ri14.imm) << 2;
		return snprintf(buf, len, "stptr.d %s, %s, %d",
			reg_name(instr.ri14.rd), reg_name(instr.ri14.rj), imm);
	}

	// === Floating-point Load/Store Instructions ===

	static int FLD_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "fld.s $f%d, %s, %d",
			instr.ri12.rd, reg_name(instr.ri12.rj), imm);
	}

	static int FST_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "fst.s $f%d, %s, %d",
			instr.ri12.rd, reg_name(instr.ri12.rj), imm);
	}

	static int FLD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "fld.d $f%d, %s, %d",
			instr.ri12.rd, reg_name(instr.ri12.rj), imm);
	}

	static int FST_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
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

	static int FSTX_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "fstx.d $fa%u, %s, %s",
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
		int32_t offset = InstructionHelpers<W>::sign_extend_21(instr.ri21.offs_lo, instr.ri21.offs_hi) << 2;
		addr_t target = pc + offset;
		return snprintf(buf, len, "beqz %s, 0x%lx",
			reg_name(instr.ri21.rj), (unsigned long)target);
	}

	static int BNEZ(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t pc) {
		// BNEZ uses ri21 format: rj at bits[9:5], 21-bit offset split across bits[25:10] and [4:0]
		int32_t offset = InstructionHelpers<W>::sign_extend_21(instr.ri21.offs_lo, instr.ri21.offs_hi) << 2;
		addr_t target = pc + offset;
		return snprintf(buf, len, "bnez %s, 0x%lx",
			reg_name(instr.ri21.rj), (unsigned long)target);
	}

	static int BEQ(char* buf, size_t len, const cpu_t& cpu, la_instruction instr, addr_t pc) {
		int32_t offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
		addr_t target = pc + offset;
		return snprintf(buf, len, "beq %s, %s, 0x%lx",
			reg_name(instr.ri16.rj), reg_name(instr.ri16.rd), (unsigned long)target);
	}

	static int BNE(char* buf, size_t len, const cpu_t& cpu, la_instruction instr, addr_t pc) {
		int32_t offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
		addr_t target = pc + offset;
		return snprintf(buf, len, "bne %s, %s, 0x%lx",
			reg_name(instr.ri16.rj), reg_name(instr.ri16.rd), (unsigned long)target);
	}

	static int BLT(char* buf, size_t len, const cpu_t& cpu, la_instruction instr, addr_t pc) {
		int32_t offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
		addr_t target = pc + offset;
		return snprintf(buf, len, "blt %s, %s, 0x%lx",
			reg_name(instr.ri16.rj), reg_name(instr.ri16.rd), (unsigned long)target);
	}

	static int BGE(char* buf, size_t len, const cpu_t& cpu, la_instruction instr, addr_t pc) {
		int32_t offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
		addr_t target = pc + offset;
		return snprintf(buf, len, "bge %s, %s, 0x%lx",
			reg_name(instr.ri16.rj), reg_name(instr.ri16.rd), (unsigned long)target);
	}

	static int BLTU(char* buf, size_t len, const cpu_t& cpu, la_instruction instr, addr_t pc) {
		int32_t offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
		addr_t target = pc + offset;
		return snprintf(buf, len, "bltu %s, %s, 0x%lx",
			reg_name(instr.ri16.rj), reg_name(instr.ri16.rd), (unsigned long)target);
	}

	static int BGEU(char* buf, size_t len, const cpu_t& cpu, la_instruction instr, addr_t pc) {
		int32_t offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
		addr_t target = pc + offset;
		return snprintf(buf, len, "bgeu %s, %s, 0x%lx",
			reg_name(instr.ri16.rj), reg_name(instr.ri16.rd), (unsigned long)target);
	}

	static int B(char* buf, size_t len, const cpu_t& cpu, la_instruction instr, addr_t pc) {
		int32_t offset = InstructionHelpers<W>::sign_extend_26(instr.i26.offs()) << 2;
		addr_t target = pc + offset;
		return snprintf(buf, len, "b 0x%lx", (unsigned long)target);
	}

	static int BL(char* buf, size_t len, const cpu_t& cpu, la_instruction instr, addr_t pc) {
		int32_t offset = InstructionHelpers<W>::sign_extend_26(instr.i26.offs()) << 2;
		addr_t target = pc + offset;
		return snprintf(buf, len, "bl 0x%lx", (unsigned long)target);
	}

	static int JIRL(char* buf, size_t len, const cpu_t& cpu, la_instruction instr, addr_t pc) {
		int32_t offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
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
		return snprintf(buf, len, "lu12i.w %s, 0x%x",
			reg_name(instr.ri20.rd), instr.ri20.imm);
	}

	static int LU32I_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "lu32i.d %s, 0x%x",
			reg_name(instr.ri20.rd), instr.ri20.imm);
	}

	static int PCADDI(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t pc) {
		int32_t si20 = InstructionHelpers<W>::sign_extend_20(instr.ri20.imm);
		int64_t offset = (int64_t)(si20 << 2);
		addr_t target = pc + offset;
		return snprintf(buf, len, "pcaddi %s, %d  # 0x%lx",
			reg_name(instr.ri20.rd), si20, (unsigned long)target);
	}

	static int PCADDU12I(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t pc) {
		int32_t si20 = InstructionHelpers<W>::sign_extend_20(instr.ri20.imm);
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
		int32_t si20 = InstructionHelpers<W>::sign_extend_20(instr.ri20.imm);
		int64_t offset = (int64_t)(si20 << 18);
		addr_t target = pc + offset;
		return snprintf(buf, len, "pcaddu18i %s, 0x%x  # 0x%lx",
			reg_name(instr.ri20.rd), si20 & 0xFFFFF, (unsigned long)target);
	}

	static int LU52I_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		const int16_t signed_imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "lu52i.d %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), signed_imm);
	}

	// === Bit Manipulation Instructions ===

	static int BSTRINS_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t msbd = (instr.whole >> 16) & 0x3F;
		uint32_t lsbd = (instr.whole >> 10) & 0x3F;
		return snprintf(buf, len, "bstrins.d %s, %s, %u, %u",
			reg_name(instr.ri16.rd), reg_name(instr.ri16.rj), msbd, lsbd);
	}

	static int BSTRPICK_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t msbd = (instr.whole >> 16) & 0x3F;
		uint32_t lsbd = (instr.whole >> 10) & 0x3F;
		return snprintf(buf, len, "bstrpick.d %s, %s, %u, %u",
			reg_name(instr.ri16.rd), reg_name(instr.ri16.rj), msbd, lsbd);
	}

	static int BSTRPICK_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t msbw = (instr.whole >> 16) & 0x1F;
		uint32_t lsbw = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "bstrpick.w %s, %s, 0x%x, 0x%x",
			reg_name(instr.ri16.rd), reg_name(instr.ri16.rj), msbw, lsbw);
	}

	// === System Instructions ===

	static int SYSCALL(char* buf, size_t len, const cpu_t&, la_instruction, addr_t) {
		return snprintf(buf, len, "syscall");
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
		int32_t imm = InstructionHelpers<W>::sign_extend_14(instr.ri14.imm) << 2;
		return snprintf(buf, len, "ll.w %s, %s, %d",
			reg_name(instr.ri14.rd), reg_name(instr.ri14.rj), imm);
	}

	static int LL_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_14(instr.ri14.imm) << 2;
		return snprintf(buf, len, "ll.d %s, %s, %d",
			reg_name(instr.ri14.rd), reg_name(instr.ri14.rj), imm);
	}

	static int SC_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_14(instr.ri14.imm) << 2;
		return snprintf(buf, len, "sc.w %s, %s, %d",
			reg_name(instr.ri14.rd), reg_name(instr.ri14.rj), imm);
	}

	static int SC_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_14(instr.ri14.imm) << 2;
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
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "slti %s, %s, %d",
			reg_name(instr.ri12.rd), reg_name(instr.ri12.rj), imm);
	}

	static int SLTUI(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
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
			reg_name(instr.r3.rd), reg_name(instr.r3.rj));
	}

	static int BITREV_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "bitrev.d %s, %s",
			reg_name(instr.r3.rd), reg_name(instr.r3.rj));
	}

	// === ALSL.W ===

	static int ALSL_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		return snprintf(buf, len, "alsl.w %s, %s, %s, %u",
			reg_name(instr.r3sa2.rd), reg_name(instr.r3sa2.rj),
			reg_name(instr.r3sa2.rk), instr.r3sa2.sa2);
	}

	// === LSX Vector Load/Store ===

	static int VLD(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "vld $vr%u, %s, %d",
			instr.ri12.rd, reg_name(instr.ri12.rj), imm);
	}

	static int VST(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "vst $vr%u, %s, %d",
			instr.ri12.rd, reg_name(instr.ri12.rj), imm);
	}

	static int XVLD(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "xvld $xr%u, %s, %d",
			instr.ri12.rd, reg_name(instr.ri12.rj), imm);
	}

	static int XVST(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
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

	static int VSEQI_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		int32_t si5 = (instr.whole >> 10) & 0x1F;
		if (si5 & 0x10) si5 |= 0xFFFFFFE0;
		return snprintf(buf, len, "vseqi.b $vr%u, $vr%u, %d", vd, vj, si5);
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

	static int FTINTRZ_W_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "ftintrz.w.d $fa%u, $fa%u", fd, fj);
	}

	static int FADD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;
		return snprintf(buf, len, "fadd.d $fa%u, $fa%u, $fa%u", fd, fj, fk);
	}

	static int FMUL_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;
		return snprintf(buf, len, "fmul.d $fa%u, $fa%u, $fa%u", fd, fj, fk);
	}

	static int FSUB_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;
		return snprintf(buf, len, "fsub.d $fa%u, $fa%u, $fa%u", fd, fj, fk);
	}

	static int FDIV_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;
		return snprintf(buf, len, "fdiv.d $fa%u, $fa%u, $fa%u", fd, fj, fk);
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

	static int VFRSTPI_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t ui5 = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vfrstpi.b $vr%u, $vr%u, 0x%x", vd, vj, ui5);
	}

	static int VPICKVE2GR_BU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t rd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t ui4 = (instr.whole >> 10) & 0xF;
		return snprintf(buf, len, "vpickve2gr.bu %s, $vr%u, 0x%x", reg_name(rd), vj, ui4);
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

	static int VPICKVE2GR_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t rd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t ui1 = (instr.whole >> 10) & 0x1;
		return snprintf(buf, len, "vpickve2gr.d %s, $vr%u, %u",
			reg_name(rd), vj, ui1);
	}

	static int VPICKVE2GR_DU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t rd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t ui1 = (instr.whole >> 10) & 0x1;
		return snprintf(buf, len, "vpickve2gr.du %s, $vr%u, %u",
			reg_name(rd), vj, ui1);
	}

	static int VPICKVE2GR_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t rd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t ui2 = (instr.whole >> 10) & 0x3;
		return snprintf(buf, len, "vpickve2gr.w %s, $vr%u, %u",
			reg_name(rd), vj, ui2);
	}

	static int VPICKVE2GR_WU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t rd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t ui2 = (instr.whole >> 10) & 0x3;
		return snprintf(buf, len, "vpickve2gr.wu %s, $vr%u, %u",
			reg_name(rd), vj, ui2);
	}

	// === LSX Vector Arithmetic/Logic ===

	static int VSUB_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vsub.b $vr%u, $vr%u, $vr%u", vd, vj, vk);
	}

	static int VSUB_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vsub.w $vr%u, $vr%u, $vr%u", vd, vj, vk);
	}

	static int VSEQ_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vseq.b $vr%u, $vr%u, $vr%u", vd, vj, vk);
	}

	static int VSLT_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vslt.b $vr%u, $vr%u, $vr%u", vd, vj, vk);
	}

	static int VILVL_H(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vilvl.h $vr%u, $vr%u, $vr%u", vd, vj, vk);
	}

	static int VILVL_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vilvl.d $vr%u, $vr%u, $vr%u", vd, vj, vk);
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

	static int VREPLGR2VR_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t rj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "vreplgr2vr.b $vr%u, %s", vd, reg_name(rj));
	}

	static int VADDI_BU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t imm = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vaddi.bu $vr%u, $vr%u, 0x%x", vd, vj, imm);
	}

	static int VADD_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vadd.b $vr%u, $vr%u, $vr%u", vd, vj, vk);
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

	static int VMIN_BU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "vmin.bu $vr%u, $vr%u, $vr%u", vd, vj, vk);
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

	static int XVFMADD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "xvfmadd.d $xr%u, $xr%u, $xr%u, $xr%u", xd, xj, xk, xa);
	}

	static int XVFMSUB_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "xvfmsub.d $xr%u, $xr%u, $xr%u, $xr%u", xd, xj, xk, xa);
	}

	static int XVFNMADD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "xvfnmadd.d $xr%u, $xr%u, $xr%u, $xr%u", xd, xj, xk, xa);
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
