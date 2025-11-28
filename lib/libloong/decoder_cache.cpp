#include "decoded_exec_segment.hpp"
#include "cpu.hpp"
#include "la_instr.hpp"
#include "threaded_bytecodes.hpp"
#include <cstring>

namespace loongarch
{
	template <int W>
	extern uint32_t optimize_bytecode(uint8_t& bytecode, address_type<W> pc, uint32_t instruction_bits);

	// Check if an instruction is diverging (changes control flow)
	// Note: PC-reading instructions (PCADDI, PCALAU12I, PCADDU12I) are NOT diverging
	// because they only read PC, they don't modify it
	template <int W>
	static bool is_diverging_instruction(uint32_t instr)
	{
		const uint32_t op6 = (instr >> 26) & 0x3F;

		switch (op6) {
		// PC-relative instructions (need current PC value)
		case 0x06: // PCADDI (0x18) / PCALAU12I (0x1A)
		case 0x07: // PCADDU12I (0x1C000000)
		// Branches and jumps
		case 0x10: // BEQZ (0x40000000)
		case 0x11: // BNEZ (0x44000000)
		case 0x12: // BCEQZ/BCNEZ (0x48000xxx)
		case 0x13: // JIRL (0x4C000000)
		case 0x14: // B (0x50000000)
		case 0x15: // BL (0x54000000)
		case 0x16: // BEQ (0x58000000)
		case 0x17: // BNE (0x5C000000)
		case 0x18: // BLT (0x60000000)
		case 0x19: // BGE (0x64000000)
		case 0x1A: // BLTU (0x68000000)
		case 0x1B: // BGEU (0x6C000000)
			return true;
		}

		// System calls (exact match required)
		if (instr == Opcode::SYSCALL) return true;
		if (instr == Opcode::BREAK) return true;

		return false;
	}

	// Determine the bytecode for a given instruction
	template <int W>
	static uint8_t determine_bytecode(uint32_t instr)
	{
		// Check for system call first
		if (instr == Opcode::SYSCALL) {
			return LA64_BC_SYSCALL;
		}

		// Check for popular instructions and assign specific bytecodes
		// Following libriscv pattern: decode based on opcode bits
		const uint32_t op10 = (instr >> 22) & 0x3FF;  // bits[31:22]
		const uint32_t op17 = (instr >> 15) & 0x1FFFF; // bits[31:15]
		const uint32_t op7 = (instr >> 25) & 0x7F;
		const uint32_t op6 = (instr >> 26);

		// LD.D: op10 = 0x0A3 (0x28C00000)
		if (op10 == 0x0A3) {
			return LA64_BC_LD_D;
		}
		// ST.D: op10 = 0x0A7 (0x29C00000)
		if (op10 == 0x0A7) {
			return LA64_BC_ST_D;
		}
		// ADDI.W: op10 = 0x00A (0x02800000)
		if (op10 == 0x00A) {
			return LA64_BC_ADDI_W;
		}
		// ADDI.D: op10 = 0x00B (0x02C00000)
		if (op10 == 0x00B) {
			return LA64_BC_ADDI_D;
		}
		// OR: op17 = 0x0002a (0x00150000)
		// MOVE is OR with rj==0: OR rd, zero, rk
		if (op17 == 0x0002a) {
			const uint32_t rj = (instr >> 5) & 0x1F;
			if (rj == 0) {
				return LA64_BC_MOVE;
			}
			return LA64_BC_OR;
		}
		// AND: op17 = 0x00029 (0x00148000)
		if (op17 == 0x00029) {
			return LA64_BC_AND;
		}
		// ANDI: op10 = 0x00D (0x03400000)
		if (op10 == 0x00D) {
			return LA64_BC_ANDI;
		}
		// ADD.D: op17 = 0x00021 (0x00108000)
		if (op17 == 0x00021) {
			return LA64_BC_ADD_D;
		}
		// SUB.D: op17 = 0x00023 (0x00118000)
		if (op17 == 0x00023) {
			return LA64_BC_SUB_D;
		}
		// ALSL.D: bits[31:18] = 0x000B with sa2 in bits[16:15]
		if ((instr & 0xFFFC0000) == 0x002C0000) {
			return LA64_BC_ALSL_D;
		}
		// ORI: op10 = 0x00E (0x03800000)
		if (op10 == 0x00E) {
			return LA64_BC_ORI;
		}
		// SLLI.W: op17 = 0x00081 (0x00408000)
		if (op17 == 0x00081) {
			return LA64_BC_SLLI_W;
		}
		// SLLI.D: op17 = 0x00082 (0x00410000)
		if (op17 == 0x00082) {
			return LA64_BC_SLLI_D;
		}
		// SRLI.D: op17 = 0x0008A (0x00450000 >> 15)
		if (op17 == 0x0008A) {
			return LA64_BC_SRLI_D;
		}
		// LD.BU: op10 = 0x0A8 (0x2a000000)
		if (op10 == 0x0A8) {
			return LA64_BC_LD_BU;
		}
		// ST.B: op10 = 0x0A4 (0x29000000)
		if (op10 == 0x0A4) {
			return LA64_BC_ST_B;
		}
		// ST.W: op10 = 0x0A6 (0x29800000)
		if (op10 == 0x0A6) {
			return LA64_BC_ST_W;
		}
		// PCADDI: op7 = 0x0C (check bits[31:25] = 0x18 >> 1)
		if (op7 == 0x0C) {
			return LA64_BC_PCADDI;
		}
		// PCALAU12I: op7 = 0x0D (check bits[31:25] = 0x1A >> 1)
		if (op7 == 0x0D) {
			return LA64_BC_PCALAU12I;
		}
		// LDPTR.D: op10 = 0x098 (0x26000000)
		if (op10 == 0x098) {
			return LA64_BC_LDPTR_D;
		}
		// LDPTR.W: op10 = 0x090 (0x24000000)
		if (op10 == 0x090) {
			return LA64_BC_LDPTR_W;
		}
		// STPTR.D: op10 = 0x09C (0x27000000)
		if (op10 == 0x09C) {
			return LA64_BC_STPTR_D;
		}
		// LU12I.W: op7 = 0x0A (check bits[31:25] = 0x14 >> 1)
		if (op7 == 0x0A) {
			return LA64_BC_LU12I_W;
		}
		// BSTRPICK.D: op10 = 0x003 (0x00c00000)
		if (op10 == 0x003) {
			return LA64_BC_BSTRPICK_D;
		}
		// LD.B: op10 = 0x0A0 (0x28000000)
		if (op10 == 0x0A0) {
			return LA64_BC_LD_B;
		}
		// STPTR.W: op10 = 0x094 (0x25000000)
		if (op10 == 0x094) {
			return LA64_BC_STPTR_W;
		}
		// LDX.D: op17 = 0x7018 (0x380C0000 >> 15)
		if (op17 == 0x7018) {
			return LA64_BC_LDX_D;
		}
		// MASKEQZ: op17 = 0x00026 (0x00130000 >> 15)
		if (op17 == 0x00026) {
			return LA64_BC_MASKEQZ;
		}
		// MASKNEZ: op17 = 0x00027 (0x00138000 >> 15)
		if (op17 == 0x00027) {
			return LA64_BC_MASKNEZ;
		}
		// MUL.D: op17 = 0x0003B (0x001d8000 >> 15)
		if (op17 == 0x0003B) {
			return LA64_BC_MUL_D;
		}
		// SUB.W: op17 = 0x00022 (0x00110000 >> 15)
		if (op17 == 0x00022) {
			return LA64_BC_SUB_W;
		}
		// SLL.D: op17 = 0x00031 (0x00188000 >> 15)
		if (op17 == 0x00031) {
			return LA64_BC_SLL_D;
		}
		// STX.D: op17 = 0x7038 (0x381C0000 >> 15)
		if (op17 == 0x7038) {
			return LA64_BC_STX_D;
		}
		// BSTRPICK.W: op11 = 0x003 (bits[31:21])
		const uint32_t op11 = (instr >> 21) & 0x7FF;
		if (op11 == 0x003) {
			return LA64_BC_BSTRPICK_W;
		}
		// SLTU: op17 = 0x00025 (0x00128000 >> 15)
		if (op17 == 0x00025) {
			return LA64_BC_SLTU;
		}
		// LDX.W: op17 = 0x7010 (0x38080000 >> 15)
		if (op17 == 0x7010) {
			return LA64_BC_LDX_W;
		}
		// STX.W: op17 = 0x7030 (0x38180000 >> 15)
		if (op17 == 0x7030) {
			return LA64_BC_STX_W;
		}
		// XOR: op17 = 0x0002B (0x00158000 >> 15)
		if (op17 == 0x0002B) {
			return LA64_BC_XOR;
		}
		// LD.HU: op10 = 0x0A9 (0x2a400000 >> 22)
		if (op10 == 0x0A9) {
			return LA64_BC_LD_HU;
		}
		// ADD.W: op17 = 0x00020 (0x00100000 >> 15)
		if (op17 == 0x00020) {
			return LA64_BC_ADD_W;
		}
		// SRAI.D: op16 = 0x0049 (bits[31:16])
		const uint32_t op16 = (instr >> 16) & 0xFFFF;
		if (op16 == 0x0049) {
			return LA64_BC_SRAI_D;
		}
		// EXT.W.B: op22 = 0x000017 (bits[31:10])
		const uint32_t op22 = (instr >> 10) & 0x3FFFFF;
		if (op22 == 0x000017) {
			return LA64_BC_EXT_W_B;
		}
		// LDX.BU: bits[31:15] = 0x7040 (0x38200000 >> 15)
		if (op17 == 0x7040) {
			return LA64_BC_LDX_BU;
		}
		// BSTRINS.D: op10 = 0x002 (0x00800000 >> 22)
		if (op10 == 0x002) {
			return LA64_BC_BSTRINS_D;
		}
		// LU32I.D: op7 = 0x0B (0x16000000 >> 25)
		if (op7 == 0x0B) {
			return LA64_BC_LU32I_D;
		}
		// CLO.W, CLZ.W, CLO.D, CLZ.D, REVB.2H: op22_val = bits[31:10]
		const uint32_t op22_val = (instr >> 10) & 0x3FFFFF;
		if (op22_val == 0x000004) return LA64_BC_CLO_W;
		if (op22_val == 0x000005) return LA64_BC_CLZ_W;
		if (op22_val == 0x000008) return LA64_BC_CLO_D;
		if (op22_val == 0x000009) return LA64_BC_CLZ_D;
		if (op22_val == 0x00000C) return LA64_BC_REVB_2H;
		// BYTEPICK.D: bits[31:18] = 0x0003
		if ((instr & 0xFFFC0000) == 0x000C0000) {
			return LA64_BC_BYTEPICK_D;
		}
		// SLTI: op10 = 0x008 (0x02000000 >> 22)
		if (op10 == 0x008) {
			return LA64_BC_SLTI;
		}
		// ST.H: op10 = 0x0A5 (0x29400000 >> 22)
		if (op10 == 0x0A5) {
			return LA64_BC_ST_H;
		}
		// FLD.D: op10 = 0x0AE (0x2B800000 >> 22)
		if (op10 == 0x0AE) {
			return LA64_BC_FLD_D;
		}
		// FST.D: op10 = 0x0AF (0x2BC00000 >> 22)
		if (op10 == 0x0AF) {
			return LA64_BC_FST_D;
		}
		// FADD.D: op17 = 0x00202 (0x01010000 >> 15)
		if (op17 == 0x00202) {
			return LA64_BC_FADD_D;
		}
		// FMUL.D: op17 = 0x0020A (0x01050000 >> 15)
		if (op17 == 0x0020A) {
			return LA64_BC_FMUL_D;
		}

		// FLDX.D: Floating-point indexed load double - op17 = 0x7068 (0x38340000 >> 15)
		if (op17 == 0x7068) {
			return LA64_BC_FLDX_D;
		}
		// FSTX.D: Floating-point indexed store double - op17 = 0x7078 (0x383C0000 >> 15)
		if (op17 == 0x7078) {
			return LA64_BC_FSTX_D;
		}

		// 4R-type format instructions: check bits[31:20] for opcode
		const uint32_t op12 = (instr >> 20) & 0xFFF;
		// FMADD.D: Scalar floating-point FMA (bits[31:20] = 0x082)
		if (op12 == 0x082) {
			return LA64_BC_FMADD_D;
		}
		// VFMADD.D: Vector FMA (bits[31:20] = 0x092)
		if (op12 == 0x092) {
			return LA64_BC_VFMADD_D;
		}
		// NOTE: VFNMADD.D is NOT 0x082! That conflicts with FMADD.D.
		// VFNMADD.D encoding needs to be verified from LoongArch manual.
		// XVFMADD.D: LASX Vector FMA (bits[31:20] = 0x0A2) - Estimated
		if (op12 == 0x0A2) {
			return LA64_BC_XVFMADD_D;
		}

		// VHADDW.D.W: Vector horizontal add with widening - op10 = 0x1C1
		if (op10 == 0x1C1) {
			return LA64_BC_VHADDW_D_W;
		}

		// XVLD: LASX 256-bit vector load - op10 = 0x0b2 (0x2C800000 >> 22)
		if (op10 == 0x0b2) {
			return LA64_BC_XVLD;
		}
		// XVST: LASX 256-bit vector store - op10 = 0x0b3 (0x2CC00000 >> 22)
		if (op10 == 0x0b3) {
			return LA64_BC_XVST;
		}

		// New bytecodes from coremark profiling
		// SRLI.W: op16 = 0x0044 (0x00448000 >> 16)
		if (op16 == 0x0044) {
			return LA64_BC_SRLI_W;
		}
		// SRL.D: op17 = 0x00032 (0x00190000 >> 15)
		if (op17 == 0x00032) {
			return LA64_BC_SRL_D;
		}
		// LU52I.D: op10 = 0x00C (0x03000000 >> 22)
		if (op10 == 0x00C) {
			return LA64_BC_LU52I_D;
		}
		// XORI: op10 = 0x00F (0x03C00000 >> 22)
		if (op10 == 0x00F) {
			return LA64_BC_XORI;
		}
		// SLTUI: op10 = 0x009 (0x02400000 >> 22)
		if (op10 == 0x009) {
			return LA64_BC_SLTUI;
		}
		// LD.H: op10 = 0x0A1 (0x28400000 >> 22)
		if (op10 == 0x0A1) {
			return LA64_BC_LD_H;
		}
		// LDX.HU: op17 = 0x7048 (0x38240000 >> 15)
		if (op17 == 0x7048) {
			return LA64_BC_LDX_HU;
		}
		// LD.WU: op10 = 0x0AA (0x2a800000 >> 22)
		if (op10 == 0x0AA) {
			return LA64_BC_LD_WU;
		}
		// PCADDU12I: op7 = 0x0E (0x1C000000 >> 25)
		if (op7 == 0x0E) {
			return LA64_BC_PCADDU12I;
		}
		// ANDN: op17 = 0x0002D (0x00168000 >> 15)
		if (op17 == 0x0002D) {
			return LA64_BC_ANDN;
		}
		// STX.B: op17 = 0x7020 (0x38100000 >> 15)
		if (op17 == 0x7020) {
			return LA64_BC_STX_B;
		}
		// CTZ.D, CTO.W, CTO.D, EXT.W.H, REVB.4H: op22_val = bits[31:10]
		if (op22_val == 0x00000B) return LA64_BC_CTZ_D;
		if (op22_val == 0x000006) return LA64_BC_CTO_W;
		if (op22_val == 0x00000A) return LA64_BC_CTO_D;
		if (op22_val == 0x000016) return LA64_BC_EXT_W_H;
		if (op22_val == 0x00000D) return LA64_BC_REVB_4H;
		// LDX.B: op17 = 0x7000 (0x38000000 >> 15)
		if (op17 == 0x7000) {
			return LA64_BC_LDX_B;
		}
		// SLT: op17 = 0x00024 (0x00120000 >> 15)
		if (op17 == 0x00024) {
			return LA64_BC_SLT;
		}
		// ORN: op17 = 0x0002C (0x00160000 >> 15)
		if (op17 == 0x0002C) {
			return LA64_BC_ORN;
		}
		// MUL.W: op17 = 0x00038 (0x001c0000 >> 15)
		if (op17 == 0x00038) {
			return LA64_BC_MUL_W;
		}
		// MOD.DU: op17 = 0x00047 (0x00238000 >> 15)
		if (op17 == 0x00047) {
			return LA64_BC_MOD_DU;
		}
		// LSX (SIMD) instructions
		// VLD: op10 = 0x0b0 (0x2c000000)
		if (op10 == 0x0b0) {
			return LA64_BC_VLD;
		}
		// VST: op10 = 0x0b1 (0x2c400000)
		if (op10 == 0x0b1) {
			return LA64_BC_VST;
		}
		// VLDX: op17 = 0x7080 (0x38400000 >> 15)
		if (op17 == 0x7080) {
			return LA64_BC_VLDX;
		}
		// VSTX: op17 = 0x7088 (0x38440000 >> 15)
		if (op17 == 0x7088) {
			return LA64_BC_VSTX;
		}
		// XVLDX: op17 = 0x7090 (0x38480000 >> 15)
		if (op17 == 0x7090) {
			return LA64_BC_XVLDX;
		}
		// XVSTX: op17 = 0x7098 (0x384C0000 >> 15)
		if (op17 == 0x7098) {
			return LA64_BC_XVSTX;
		}
		// VFADD.D: op17 = 0xe262 (0x71310000 >> 15)
		if (op17 == 0xe262) {
			return LA64_BC_VFADD_D;
		}
		// XVFADD.D: op17 = 0xea62 (0x75310000 >> 15)
		if (op17 == 0xea62) {
			return LA64_BC_XVFADD_D;
		}
		// XVFMUL.D: op17 = 0xea72 (0x75390000 >> 15)
		if (op17 == 0xea72) {
			return LA64_BC_XVFMUL_D;
		}
		// XVFMSUB.D and XVFNMADD.D: Check for 4R-type with op12 = 0x0CA or 0x0D2
		if (op6 == 0x03) {
			uint32_t op12 = (instr >> 20) & 0xfff;
			if (op12 == 0x0ca) return LA64_BC_XVFMSUB_D;
			if (op12 == 0x0d2) return LA64_BC_XVFNMADD_D;
		}
		// XVORI.B: op17 = 0xefa8 (0x77d40000 >> 15)
		if (op17 == 0xefa8) {
			return LA64_BC_XVORI_B;
		}
		// XVXORI.B: op17 = 0xefd9 (0x77ec0000 >> 15)
		if (op17 == 0xefd9) {
			return LA64_BC_XVXORI_B;
		}
		// XVILVL.D: op17 = 0xea37 (0x751b8000 >> 15)
		if (op17 == 0xea37) {
			return LA64_BC_XVILVL_D;
		}
		// XVILVH.D: op17 = 0xea3f (0x751f8000 >> 15)
		if (op17 == 0xea3f) {
			return LA64_BC_XVILVH_D;
		}
		// XVPERMI.D: op14 = 0x1dc1 (0x7707e000 >> 18)
		if ((instr >> 18) == 0x1dc1) {
			return LA64_BC_XVPERMI_D;
		}
		// XVPACKEV.D: op17 = 0xea66 (0x75330000 >> 15)
		if (op17 == 0xea66) {
			return LA64_BC_XVPACKEV_D;
		}
		// XVPACKOD.D: op17 = 0xee33 (0x77198000 >> 15)
		if (op17 == 0xee33) {
			return LA64_BC_XVPACKOD_D;
		}
		// XVPICKEV.D: op17 = 0xee07 (0x7703c000 >> 15)
		if (op17 == 0xee07) {
			return LA64_BC_XVPICKEV_D;
		}

		// Branch instructions
		if (op6 == 0x10) { // BEQZ
			return LA64_BC_BEQZ;
		}
		if (op6 == 0x11) { // BNEZ
			return LA64_BC_BNEZ;
		}
		// BCEQZ/BCNEZ: op6 = 0x12 (0x48000000)
		if (op6 == 0x12) {
			// Distinguish BCEQZ (bits[8:8] = 0) vs BCNEZ (bits[8:8] = 1)
			if ((instr & 0x00000300) == 0x00000000) {
				return LA64_BC_BCEQZ;
			} else if ((instr & 0x00000300) == 0x00000100) {
				return LA64_BC_BCNEZ;
			}
		}
		if (op6 == 0x16) { // BEQ
			return LA64_BC_BEQ;
		}
		if (op6 == 0x17) { // BNE
			return LA64_BC_BNE;
		}
		// JIRL: op6 = 0x13 (0x4C000000)
		if (op6 == 0x13) {
			return LA64_BC_JIRL;
		}
		if (op6 == 0x14) { // B
			return LA64_BC_B;
		}
		if (op6 == 0x15) { // BL
			return LA64_BC_BL;
		}
		if (op6 == 0x18) { // BLT
			return LA64_BC_BLT;
		}
		if (op6 == 0x19) { // BGE
			return LA64_BC_BGE;
		}
		if (op6 == 0x1A) { // BLTU
			return LA64_BC_BLTU;
		}
		if (op6 == 0x1B) { // BGEU
			return LA64_BC_BGEU;
		}

		// Fallback: Check if it's diverging (PC-modifying)
		if (is_diverging_instruction<W>(instr)) {
			return LA64_BC_FUNCBLOCK;
		}

		// Fallback: regular function (non-PC-modifying)
		return LA64_BC_FUNCTION;
	}

	// Populate decoder cache for an execute segment
	template <int W>
	void populate_decoder_cache(DecodedExecuteSegment<W>& segment,
		address_type<W> exec_begin, const uint8_t* code, size_t code_size)
	{
		// Round down to nearest instruction boundary (4 bytes)
		// This safely handles segments where .text + .rodata are merged
		const size_t aligned_size = code_size & ~size_t(3);
		if (aligned_size == 0) {
			// No complete instructions to cache
			segment.set_decoder_cache(nullptr, 0);
			return;
		}

		const size_t num_instructions = aligned_size / 4;
		auto* cache = new DecoderData<W>[num_instructions + 1];
		// Guarantee that invalid instruction is handler 0
		const auto invalid_handler = DecoderData<W>::compute_handler_for(0);
		if (invalid_handler != 0) {
			// This should never happen, but just in case
			throw std::runtime_error("DecoderCache: Handler 0 is not invalid handler");
		}

		// Scan backwards to calculate block_bytes
		// This computes how many bytes until the next diverging instruction
		const uint32_t* instr_ptr = reinterpret_cast<const uint32_t*>(code);
		uint32_t accumulated_bytes = 0;
		std::unordered_map<typename DecoderData<W>::handler_t, uint8_t> handler_map;
		for (size_t i = num_instructions; i-- > 0; ) {
			const uint32_t instr = instr_ptr[i];

			// Decode and cache the handler for fast dispatch
			const auto& decoded = CPU<W>::decode(la_instruction{instr});
			auto it = handler_map.find(decoded.handler);
			if (it != handler_map.end()) {
				// Existing handler
				cache[i].handler_idx = it->second;
			} else {
				// New handler
				const uint8_t handler_idx = DecoderData<W>::compute_handler_for(decoded.handler);
				handler_map.insert_or_assign(decoded.handler, handler_idx);
				cache[i].handler_idx = handler_idx;
			}

			// Set bytecode for threaded dispatch
			cache[i].bytecode = determine_bytecode<W>(instr);
			// Optimize instruction bits for popular bytecodes
			// The optimizer may also modify the bytecode if needed,
			// typically to rewrite cases where rd == zero register.
			// This avoids a check in the hot-path for rd != 0.
			const address_type<W> pc = exec_begin + (i * sizeof(la_instruction));
			cache[i].instr = optimize_bytecode<W>(cache[i].bytecode, pc, instr);

			if (is_diverging_instruction<W>(instr)) {
				// Diverging instruction: block_bytes = 0
				cache[i].block_bytes = 0;
				accumulated_bytes = 0;
			} else {
				// Non-diverging: accumulate bytes to next diverge
				accumulated_bytes += 4;
				cache[i].block_bytes = accumulated_bytes;
			}
		}
		// The final instruction in every segment must be zero (invalid)
		// This marks the end of the cache, and prevents overruns
		cache[num_instructions].instr = 0;
		cache[num_instructions].block_bytes = 0;
		cache[num_instructions].bytecode = LA64_BC_INVALID;
		cache[num_instructions].handler_idx = 0;

		// Store the cache in the segment
		segment.set_decoder_cache(cache, num_instructions);
	}

	template <int W>
	void DecodedExecuteSegment<W>::set(address_t entry_addr, const DecoderData<W>& data)
	{
		const size_t index = (entry_addr - m_exec_begin) >> DecoderCache<W>::SHIFT;
		if (index < m_decoder_cache.size) {
			m_decoder_cache.cache[index] = data;
		} else {
			fprintf(stderr,
				"DecodedExecuteSegment: set() address out of range: 0x%lx index=%zu size=%zu\n",
				long(entry_addr), index, m_decoder_cache.size);
			throw MachineException(INVALID_PROGRAM,
				"DecodedExecuteSegment: set() address out of range", entry_addr);
		}
	}

#ifdef LA_32
	template struct DecodedExecuteSegment<LA32>;
	template struct DecoderCache<LA32>;
	template struct DecoderData<LA32>;
	template void populate_decoder_cache<LA32>(DecodedExecuteSegment<LA32>&, address_type<LA32>, const uint8_t*, size_t);
#endif
#ifdef LA_64
	template struct DecodedExecuteSegment<LA64>;
	template struct DecoderCache<LA64>;
	template struct DecoderData<LA64>;
	template void populate_decoder_cache<LA64>(DecodedExecuteSegment<LA64>&, address_type<LA64>, const uint8_t*, size_t);
#endif
} // loongarch
