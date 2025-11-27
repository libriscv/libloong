#include "decoder_cache.hpp"

#include "la_instr.hpp"
#include "threaded_bytecodes.hpp"

namespace loongarch {

template <int W>
uint32_t optimize_bytecode(uint8_t& bytecode, address_type<W> pc, uint32_t instruction_bits)
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
		case LA64_BC_AND: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_ALSL_D: {
			auto fi = *(FasterLA64_R3SA2 *)&instruction_bits;
			fi.rd = original.r3sa2.rd;
			fi.rj = original.r3sa2.rj;
			fi.rk = original.r3sa2.rk;
			fi.sa2 = original.r3sa2.sa2;
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
		case LA64_BC_SRLI_D: {
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
		case LA64_BC_ST_W: {
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			return fi.whole;
		} break;
		case LA64_BC_BSTRPICK_D: {
			auto fi = *(FasterLA64_BitField *)&instruction_bits;
			fi.rd = (original.whole >> 0) & 0x1F;
			fi.rj = (original.whole >> 5) & 0x1F;
			fi.lsbd = (original.whole >> 10) & 0x3F;
			fi.msbd = (original.whole >> 16) & 0x3F;
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
		case LA64_BC_STPTR_W: {
			auto fi = *(FasterLA64_RI14 *)&instruction_bits;
			fi.rd = original.ri14.rd;
			fi.rj = original.ri14.rj;
			fi.set_imm(original.ri14.imm);
			return fi.whole;
		} break;
		case LA64_BC_LD_B: {
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			return fi.whole;
		} break;
		case LA64_BC_LDX_D: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_MASKEQZ: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_MASKNEZ: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_MUL_D: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_SUB_W: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_SLL_D: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_STX_D: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_BSTRPICK_W: {
			auto fi = *(FasterLA64_BitFieldW *)&instruction_bits;
			fi.rd = (original.whole >> 0) & 0x1F;
			fi.rj = (original.whole >> 5) & 0x1F;
			fi.lsbw = (original.whole >> 10) & 0x1F;
			fi.msbw = (original.whole >> 16) & 0x1F;
			return fi.whole;
		} break;
		case LA64_BC_SLTU: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_LDX_W: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_STX_W: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_XOR: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_LD_HU: {
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			return fi.whole;
		} break;
		case LA64_BC_ADD_W: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_SRAI_D: {
			auto fi = *(FasterLA64_Shift64 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.ui6 = (original.whole >> 10) & 0x3F;
			return fi.whole;
		} break;
		case LA64_BC_EXT_W_B: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r2.rd;
			fi.rj = original.r2.rj;
			fi.rk = 0; // Not used
			return fi.whole;
		} break;
		case LA64_BC_LDX_BU: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_BSTRINS_D: {
			auto fi = *(FasterLA64_BitField *)&instruction_bits;
			fi.rd = (original.whole >> 0) & 0x1F;
			fi.rj = (original.whole >> 5) & 0x1F;
			fi.lsbd = (original.whole >> 10) & 0x3F;
			fi.msbd = (original.whole >> 16) & 0x3F;
			return fi.whole;
		} break;
		case LA64_BC_LU32I_D: {
			auto fi = *(FasterLA64_RI20 *)&instruction_bits;
			fi.rd = original.ri20.rd;
			fi.set_imm(original.ri20.imm);
			return fi.whole;
		} break;
		case LA64_BC_JIRL:
			// No optimization needed - JIRL uses original instruction bits
			// because it needs to access ri16 fields directly in VIEW_INSTR()
			return instruction_bits;
		default:
			// No optimization
			return instruction_bits;
	}
}

#ifdef LA_32
	template uint32_t optimize_bytecode<LA32>(uint8_t&, address_type<LA32>, uint32_t);
#endif
#ifdef LA_64
	template uint32_t optimize_bytecode<LA64>(uint8_t&, address_type<LA64>, uint32_t);
#endif
} // namespace loongarch
