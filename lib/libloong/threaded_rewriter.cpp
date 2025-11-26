#include "decoder_cache.hpp"

#include "la_instr.hpp"
#include "threaded_bytecodes.hpp"

namespace loongarch {

template <int W>
uint32_t optimize_bytecode(const uint8_t bytecode, uint32_t instruction_bits)
{
	const la_instruction original{instruction_bits};

	switch (bytecode) {
		// Bytecodes with optimized field access
		case LA64_BC_LD_D: {
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			return fi.whole;
		} break;
		case LA64_BC_ST_D: {
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			return fi.whole;
		} break;
		case LA64_BC_OR: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_ADDI_W: {
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			return fi.whole;
		} break;
		case LA64_BC_ADDI_D: {
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			return fi.whole;
		} break;
		case LA64_BC_ANDI: {
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			return fi.whole;
		} break;
		case LA64_BC_ADD_D: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_SUB_D: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_ORI: {
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			return fi.whole;
		} break;
		case LA64_BC_SLLI_W: {
			auto fi = *(FasterLA64_Shift *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.ui5 = (original.whole >> 10) & 0x1F;
			return fi.whole;
		} break;
		case LA64_BC_SLLI_D: {
			auto fi = *(FasterLA64_Shift64 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.ui6 = (original.whole >> 10) & 0x3F;
			return fi.whole;
		} break;
		case LA64_BC_LD_BU: {
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			return fi.whole;
		} break;
		case LA64_BC_ST_B: {
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			return fi.whole;
		} break;
		case LA64_BC_PCADDI:
		case LA64_BC_PCALAU12I:
		case LA64_BC_LU12I_W:
			// No optimization needed - use original instruction bits
			return instruction_bits;
		case LA64_BC_LDPTR_D: {
			auto fi = *(FasterLA64_RI14 *)&instruction_bits;
			fi.rd = original.ri14.rd;
			fi.rj = original.ri14.rj;
			fi.set_imm(original.ri14.imm);
			return fi.whole;
		} break;
		case LA64_BC_LDPTR_W: {
			auto fi = *(FasterLA64_RI14 *)&instruction_bits;
			fi.rd = original.ri14.rd;
			fi.rj = original.ri14.rj;
			fi.set_imm(original.ri14.imm);
			return fi.whole;
		} break;
		case LA64_BC_STPTR_D: {
			auto fi = *(FasterLA64_RI14 *)&instruction_bits;
			fi.rd = original.ri14.rd;
			fi.rj = original.ri14.rj;
			fi.set_imm(original.ri14.imm);
			return fi.whole;
		} break;
	default:
		// No optimization
		return instruction_bits;
	}
}

#ifdef LA_32
	template uint32_t optimize_bytecode<LA32>(const uint8_t bytecode, uint32_t instruction_bits);
#endif
#ifdef LA_64
	template uint32_t optimize_bytecode<LA64>(const uint8_t bytecode, uint32_t instruction_bits);
#endif
} // namespace loongarch
