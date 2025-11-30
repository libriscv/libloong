#include "decoded_exec_segment.hpp"

#include "la_instr.hpp"
#include "threaded_bytecodes.hpp"
#define NOP_IF_RD_ZERO(fi_rd, bytecode) \
	if (fi_rd == 0) {                   \
		bytecode = LA64_BC_NOP;         \
		return original.whole;		    \
	}
#define INVALID_IF_RD_ZERO(fi_rd, bytecode) \
	if (fi_rd == 0) {                   \
		bytecode = LA64_BC_INVALID;     \
		return original.whole;		    \
	}

namespace loongarch {

template <int W>
uint32_t DecodedExecuteSegment<W>::optimize_bytecode(uint8_t& bytecode, address_t pc, uint32_t instruction_bits) const
{
	const la_instruction original{instruction_bits};

	switch (bytecode) {
		// Bytecodes with optimized field access
		case LA64_BC_B:
		case LA64_BC_BL: {
			const auto offset = InstructionHelpers<W>::sign_extend_26(original.i26.offs()) << 2;
			// This is a local branch, so it better be within the segment
			if (this->is_within(pc + offset)) {
				// The offset is the optimized instruction
				return (int32_t)offset;
			} else {
				bytecode = LA64_BC_INVALID;
				return original.whole;
			}
		} break;
		case LA64_BC_BEQ:
		case LA64_BC_BNE:
		case LA64_BC_BLT:
		case LA64_BC_BGE:
		case LA64_BC_BLTU:
		case LA64_BC_BGEU: {
			// These use ri16 format: rd, rj, offs16
			const auto offset = InstructionHelpers<W>::sign_extend_16(original.ri16.imm) << 2;
			// Validate branch target
			if (this->is_within(pc + offset)) {
				auto fi = *(FasterLA64_RI16_Branch *)&instruction_bits;
				fi.rd = original.ri16.rd;
				fi.rj = original.ri16.rj;
				fi.set_offset(offset);
				return fi.whole;
			} else {
				bytecode = LA64_BC_INVALID;
				return original.whole;
			}
		} break;
		case LA64_BC_LD_D: {
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_MOVE: {
			// MOVE is OR rd, zero, rk - we only need rd and rk
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = 0;  // Always zero for MOVE
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
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
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_AND: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_ALSL_D: {
			auto fi = *(FasterLA64_R3SA2 *)&instruction_bits;
			fi.rd = original.r3sa2.rd;
			fi.rj = original.r3sa2.rj;
			fi.rk = original.r3sa2.rk;
			fi.sa2 = original.r3sa2.sa2;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_ADDI_W: {
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_ADDI_D: {
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_ANDI: {
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			// ANDI uses zero-extended immediate, not sign-extended
			fi.imm = original.ri12.imm & 0xFFF;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_ADD_D: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_SUB_D: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_ORI: {
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.imm = original.ri12.imm;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_SLLI_W: {
			auto fi = *(FasterLA64_Shift *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.ui5 = (original.whole >> 10) & 0x1F;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_SLLI_D: {
			auto fi = *(FasterLA64_Shift64 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.ui6 = (original.whole >> 10) & 0x3F;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_SRLI_D: {
			auto fi = *(FasterLA64_Shift64 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.ui6 = (original.whole >> 10) & 0x3F;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_LD_BU: {
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			NOP_IF_RD_ZERO(fi.rd, bytecode);
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
			NOP_IF_RD_ZERO(fi.rd, bytecode);
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
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_LDPTR_W: {
			auto fi = *(FasterLA64_RI14 *)&instruction_bits;
			fi.rd = original.ri14.rd;
			fi.rj = original.ri14.rj;
			fi.set_imm(original.ri14.imm);
			NOP_IF_RD_ZERO(fi.rd, bytecode);
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
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_LDX_D: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_MASKEQZ: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_MASKNEZ: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_MUL_D: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_SUB_W: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_SLL_D: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
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
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_SLTU: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_LDX_W: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
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
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_LD_HU: {
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_ADD_W: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_SRAI_D: {
			auto fi = *(FasterLA64_Shift64 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.ui6 = (original.whole >> 10) & 0x3F;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_EXT_W_B: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r2.rd;
			fi.rj = original.r2.rj;
			fi.rk = 0; // Not used
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_LDX_BU: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_BSTRINS_D: {
			auto fi = *(FasterLA64_BitField *)&instruction_bits;
			fi.rd = (original.whole >> 0) & 0x1F;
			fi.rj = (original.whole >> 5) & 0x1F;
			fi.lsbd = (original.whole >> 10) & 0x3F;
			fi.msbd = (original.whole >> 16) & 0x3F;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_LU32I_D: {
			auto fi = *(FasterLA64_RI20 *)&instruction_bits;
			fi.rd = original.ri20.rd;
			fi.set_imm(original.ri20.imm);
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_CLO_W:
		case LA64_BC_CLZ_W:
		case LA64_BC_CLO_D:
		case LA64_BC_CLZ_D:
		case LA64_BC_REVB_2H: {
			auto fi = *(FasterLA64_R2 *)&instruction_bits;
			fi.rd = original.r2.rd;
			fi.rj = original.r2.rj;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_BYTEPICK_D: {
			auto fi = *(FasterLA64_R3SA3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			fi.sa3 = (original.whole >> 15) & 0x7;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_SLTI: {
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_ST_H: {
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			return fi.whole;
		} break;
		case LA64_BC_FLD_D: {
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			return fi.whole;
		} break;
		case LA64_BC_FST_D: {
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			return fi.whole;
		} break;
		case LA64_BC_FADD_D:
		case LA64_BC_FMUL_D: {
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		// LSX (SIMD) instructions
		case LA64_BC_VLD: {
			// VLD vd, rj, si12 - uses RI12 format
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			return fi.whole;
		} break;
		case LA64_BC_VST: {
			// VST vd, rj, si12 - uses RI12 format
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			return fi.whole;
		} break;
		case LA64_BC_VLDX: {
			// VLDX vd, rj, rk - uses R3 format
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_VSTX: {
			// VSTX vd, rj, rk - uses R3 format
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_VFADD_D: {
			// VFADD.D vd, vj, vk - uses R3 format
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_VFMADD_D: {
			// VFMADD.D vd, vj, vk, va - 4R-type format
			auto fi = *(FasterLA64_4R *)&instruction_bits;
			fi.rd = original.r4.rd;
			fi.rj = original.r4.rj;
			fi.rk = original.r4.rk;
			fi.ra = original.r4.ra;
			return fi.whole;
		} break;
		case LA64_BC_VFNMADD_D: {
			// VFNMADD.D vd, vj, vk, va - 4R-type format
			auto fi = *(FasterLA64_4R *)&instruction_bits;
			fi.rd = original.r4.rd;
			fi.rj = original.r4.rj;
			fi.rk = original.r4.rk;
			fi.ra = original.r4.ra;
			return fi.whole;
		} break;
		case LA64_BC_VHADDW_D_W: {
			// VHADDW.D.W vd, vj, vk - uses R3 format
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_XVLD: {
			// XVLD xd, rj, si12 - uses RI12 format
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			return fi.whole;
		} break;
		case LA64_BC_XVST: {
			// XVST xd, rj, si12 - uses RI12 format
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			return fi.whole;
		} break;
		case LA64_BC_XVLDX: {
			// XVLDX xd, rj, rk - uses R3 format
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_XVSTX: {
			// XVSTX xd, rj, rk - uses R3 format
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_FMADD_D: {
			// FMADD.D fd, fj, fk, fa - 4R-type format
			auto fi = *(FasterLA64_4R *)&instruction_bits;
			fi.rd = original.r4.rd;
			fi.rj = original.r4.rj;
			fi.rk = original.r4.rk;
			fi.ra = original.r4.ra;
			return fi.whole;
		} break;
		case LA64_BC_FLDX_D: {
			// FLDX.D fd, rj, rk - uses R3 format
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_FSTX_D: {
			// FSTX.D fd, rj, rk - uses R3 format
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			return fi.whole;
		} break;
		case LA64_BC_SRLI_W: {
			// SRLI.W rd, rj, ui5 - uses Shift format
			auto fi = *(FasterLA64_Shift *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.ui5 = (original.whole >> 10) & 0x1F;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_SRL_D: {
			// SRL.D rd, rj, rk - uses R3 format
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_LU52I_D: {
			// LU52I.D rd, rj, imm12 - uses RI12 format
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.imm = original.ri12.imm;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_XORI: {
			// XORI rd, rj, imm12 - uses RI12 format with zero-extended immediate
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			// XORI uses zero-extended immediate, not sign-extended
			fi.imm = original.ri12.imm & 0xFFF;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_SLTUI: {
			// SLTUI rd, rj, imm12 - uses RI12 format with zero-extended immediate
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			// SLTUI uses zero-extended immediate for unsigned comparison
			fi.imm = original.ri12.imm & 0xFFF;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_LD_H: {
			// LD.H rd, rj, imm12 - uses RI12 format
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_LDX_HU: {
			// LDX.HU rd, rj, rk - uses R3 format
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_LD_WU: {
			// LD.WU rd, rj, imm12 - uses RI12 format
			auto fi = *(FasterLA64_RI12 *)&instruction_bits;
			fi.rd = original.ri12.rd;
			fi.rj = original.ri12.rj;
			fi.set_imm(original.ri12.imm);
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_PCADDU12I:
			// PCADDU12I uses PC, handled as diverging instruction
			return instruction_bits;
		case LA64_BC_PCADDU18I:
			// PCADDU18I uses PC, handled as diverging instruction
			return instruction_bits;
		case LA64_BC_ANDN: {
			// ANDN rd, rj, rk - uses R3 format
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_STX_B: {
			// STX.B rd, rj, rk - uses R3 format
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_CTZ_D: {
			// CTZ.D rd, rj - uses R2 format
			auto fi = *(FasterLA64_R2 *)&instruction_bits;
			fi.rd = original.r2.rd;
			fi.rj = original.r2.rj;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_CTO_W: {
			// CTO.W rd, rj - uses R2 format
			auto fi = *(FasterLA64_R2 *)&instruction_bits;
			fi.rd = original.r2.rd;
			fi.rj = original.r2.rj;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_EXT_W_H: {
			// EXT.W.H rd, rj - uses R2 format
			auto fi = *(FasterLA64_R2 *)&instruction_bits;
			fi.rd = original.r2.rd;
			fi.rj = original.r2.rj;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_LDX_B: {
			// LDX.B rd, rj, rk - uses R3 format
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_SLT: {
			// SLT rd, rj, rk - uses R3 format
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_ORN: {
			// ORN rd, rj, rk - uses R3 format
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_CTO_D: {
			// CTO.D rd, rj - uses R2 format
			auto fi = *(FasterLA64_R2 *)&instruction_bits;
			fi.rd = original.r2.rd;
			fi.rj = original.r2.rj;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_MUL_W: {
			// MUL.W rd, rj, rk - uses R3 format
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_MOD_DU: {
			// MOD.DU rd, rj, rk - uses R3 format
			auto fi = *(FasterLA64_R3 *)&instruction_bits;
			fi.rd = original.r3.rd;
			fi.rj = original.r3.rj;
			fi.rk = original.r3.rk;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_REVB_4H: {
			// REVB.4H rd, rj - uses R2 format
			auto fi = *(FasterLA64_R2 *)&instruction_bits;
			fi.rd = original.r2.rd;
			fi.rj = original.r2.rj;
			NOP_IF_RD_ZERO(fi.rd, bytecode);
			return fi.whole;
		} break;
		case LA64_BC_JIRL: {
			// JIRL uses ri16 format: rd, rj, imm16
			auto fi = *(FasterLA64_RI16_Branch *)&instruction_bits;
			fi.rd = original.ri16.rd;
			fi.rj = original.ri16.rj;
			fi.offset = original.ri16.imm;
			return fi.whole;
		} break;
		case LA64_BC_BEQZ:
		case LA64_BC_BNEZ: {
			// BEQZ/BNEZ use ri21 format: rj, offs21
			const auto offset = InstructionHelpers<W>::sign_extend_21(original.ri21.offs_lo, original.ri21.offs_hi) << 2;
			// Check if branch target is within segment
			if (this->is_within(pc + offset)) {
				auto fi = *(FasterLA64_RI21_Branch *)&instruction_bits;
				fi.rj = original.ri21.rj;
				fi.set_offset(offset);
				return fi.whole;
			} else {
				bytecode = LA64_BC_INVALID;
				return original.whole;
			}
		} break;
		case LA64_BC_BCEQZ:
		case LA64_BC_BCNEZ: {
			// BCEQZ/BCNEZ use bits[7:5] for condition flag (cj), offs21 for offset
			const auto offset = InstructionHelpers<W>::sign_extend_21(original.ri21.offs_lo, original.ri21.offs_hi) << 2;
			// Check if branch target is within segment
			if (this->is_within(pc + offset)) {
				auto fi = *(FasterLA64_RI21_Branch *)&instruction_bits;
				fi.rj = (original.whole >> 5) & 0x7; // Extract cj from bits[7:5]
				fi.set_offset(offset);
				return fi.whole;
			} else {
				bytecode = LA64_BC_INVALID;
				return original.whole;
			}
		} break;
		default:
			// No optimization
			return instruction_bits;
	}
}

#ifdef LA_32
	template struct DecodedExecuteSegment<32>;
#endif
#ifdef LA_64
	template struct DecodedExecuteSegment<64>;
#endif
} // namespace loongarch
