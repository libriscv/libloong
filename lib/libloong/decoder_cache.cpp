#include "decoded_exec_segment.hpp"
#include "cpu.hpp"
#include "la_instr.hpp"
#include "threaded_bytecodes.hpp"
#include <cstring>

namespace loongarch
{
	// Check if an instruction is diverging (changes control flow)
	// Note: PC-reading instructions (PCADDI, PCALAU12I, PCADDU12I) are NOT diverging
	// because they only read PC, they don't modify it
	static bool is_diverging_instruction(uint32_t instr)
	{
		const uint32_t op6 = (instr >> 26) & 0x3F;

		switch (op6) {
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
	static uint8_t determine_bytecode(const uint32_t instr, const uint16_t handler_idx)
	{
		// Check for system call first
		if (instr == Opcode::SYSCALL) {
			return LA64_BC_SYSCALL;
		} else if (instr == Opcode::BREAK) {
			return LA64_BC_FUNCTION;
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
		// ADDI.8: op10 = 0x00A (0x02800000)
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
			// If imm is 0, this is a MOVE instruction
			const uint32_t imm = instr & 0xFFF;
			if (imm == 0) {
				return LA64_BC_MOVE;
			}
			return LA64_BC_ORI;
		}
		// SLLI.8: op17 = 0x00081 (0x00408000)
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
		// ST.8: op10 = 0x0A6 (0x29800000)
		if (op10 == 0x0A6) {
			return LA64_BC_ST_W;
		}
		// LDPTR.D: 0x26000000 (check bits[31:24] = 0x26)
		if ((instr >> 24) == 0x26) {
			return LA64_BC_LDPTR_D;
		}
		// LDPTR.8: 0x24000000 (check bits[31:24] = 0x24)
		if ((instr >> 24) == 0x24) {
			return LA64_BC_LDPTR_W;
		}
		// STPTR.D: 0x27000000 (check bits[31:24] = 0x27)
		if ((instr >> 24) == 0x27) {
			return LA64_BC_STPTR_D;
		}
		// STPTR.8: 0x25000000 (check bits[31:24] = 0x25)
		if ((instr >> 24) == 0x25) {
			return LA64_BC_STPTR_W;
		}
		// LU12I.8: op7 = 0x0A (check bits[31:25] = 0x14 >> 1)
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
		// SUB.8: op17 = 0x00022 (0x00110000 >> 15)
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
		// BSTRINS.W and BSTRPICK.W: op11 = 0x003 (bits[31:21])
		// Differentiated by bit 15: BSTRINS.W (bit 15 = 0), BSTRPICK.W (bit 15 = 1)
		const uint32_t op11 = (instr >> 21) & 0x7FF;
		if (op11 == 0x003) {
			if ((instr >> 15) & 1) {
				return LA64_BC_BSTRPICK_W;
			} else {
				// BSTRINS.W bytecode not implemented, fall through to FUNCTION
			}
		}
		// SLTU: op17 = 0x00025 (0x00128000 >> 15)
		if (op17 == 0x00025) {
			return LA64_BC_SLTU;
		}
		// LDX.W: op17 = 0x7008 (0x38040000 >> 15)
		if (op17 == 0x7008) {
			return LA64_BC_LDX_H;
		}
		// LDX.W: op17 = 0x7010 (0x38080000 >> 15)
		if (op17 == 0x7010) {
			return LA64_BC_LDX_W;
		}
		// STX.H: op17 = 0x7028 (0x38140000 >> 15)
		if (op17 == 0x7028) {
			return LA64_BC_STX_H;
		}
		// STX.8: op17 = 0x7030 (0x38180000 >> 15)
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
		// ADD.8: op17 = 0x00020 (0x00100000 >> 15)
		if (op17 == 0x00020) {
			return LA64_BC_ADD_W;
		}
		// SRAI.D: op16 = 0x0049 (bits[31:16])
		const uint32_t op16 = (instr >> 16) & 0xFFFF;
		if (op16 == 0x0049) {
			return LA64_BC_SRAI_D;
		}
		// EXT.8.B: op22 = 0x000017 (bits[31:10])
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
		// CLO.8, CLZ.8, CLO.D, CLZ.D, REVB.2H: op22_val = bits[31:10]
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

		// VHADDW.D.8: Vector horizontal add with widening - op10 = 0x1C1
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
		// SRLI.8: op16 = 0x0044 (0x00448000 >> 16)
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
		// ANDN: op17 = 0x0002D (0x00168000 >> 15)
		if (op17 == 0x0002D) {
			return LA64_BC_ANDN;
		}
		// STX.B: op17 = 0x7020 (0x38100000 >> 15)
		if (op17 == 0x7020) {
			return LA64_BC_STX_B;
		}
		// CTZ.D, CTO.8, CTO.D, EXT.8.H, REVB.4H: op22_val = bits[31:10]
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
		// MUL.8: op17 = 0x00038 (0x001c0000 >> 15)
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

		// When debugging fast dispatch, all bytecodes above this line
		// may be temporarily removed in order to eliminate a problematic
		// bytecode, but *none below this line*.

		// Non-diverging PC-modifying instructions
		// PCADDI: op7 = 0x0C (check bits[31:25] = 0x18 >> 1)
		if (op7 == 0x0C) {
			return LA64_BC_PCADDI;
		}
		// PCALAU12I: op7 = 0x0D (check bits[31:25] = 0x1A >> 1)
		if (op7 == 0x0D) {
			return LA64_BC_PCALAU12I;
		}
		// PCADDU12I: op7 = 0x0E (0x1C000000 >> 25)
		if (op7 == 0x0E) {
			return LA64_BC_PCADDU12I;
		}
		// PCADDU18I: op7 = 0x0F (0x1E000000 >> 25)
		if (op7 == 0x0F) {
			return LA64_BC_PCADDU18I;
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

		// Fallback: regular function (non-PC-modifying)
		return LA64_BC_FUNCTION + (handler_idx >> 8);
	}

	// Populate decoder cache for an execute segment
	void populate_decoder_cache(DecodedExecuteSegment& segment,
		address_t exec_begin, const uint8_t* code, size_t code_size)
	{
		// Round down to nearest instruction boundary (4 bytes)
		// This safely handles segments where .text + .rodata are merged
		const size_t aligned_size = code_size & ~size_t(3);
		if (aligned_size < 4) {
			// No complete instructions to cache
			segment.set_decoder_cache(nullptr, 0);
			return;
		}

		const size_t num_instructions = aligned_size / 4;
		auto* cache = new DecoderData[num_instructions + 1];
		// Guarantee that invalid instruction is handler 0
		const auto invalid_handler = DecoderData::compute_handler_for(
			CPU::get_invalid_instruction().handler);
		if (invalid_handler != 0) {
			// This should never happen, but just in case
			throw std::runtime_error("DecoderCache: Handler 0 is not invalid handler");
		}

		// Scan backwards to calculate block_bytes
		// This computes how many bytes until the next diverging instruction
		const uint32_t* instr_ptr = reinterpret_cast<const uint32_t*>(code);
		uint32_t accumulated_bytes = 0;
		std::unordered_map<typename DecoderData::handler_t, uint16_t> handler_map;
		for (size_t i = num_instructions; i-- > 0; ) {
			const uint32_t instr = instr_ptr[i];

			// Decode and cache the handler for fast dispatch
			const auto& decoded = CPU::decode(la_instruction{instr});
			auto it = handler_map.find(decoded.handler);
			uint16_t handler_idx = 0;
			if (it != handler_map.end()) {
				// Existing handler
				handler_idx = it->second;
			} else {
				// New handler
				handler_idx = DecoderData::compute_handler_for(decoded.handler);
				handler_map.insert_or_assign(decoded.handler, handler_idx);
			}

			// Set bytecode for threaded dispatch
			cache[i].bytecode = determine_bytecode(instr, handler_idx);
			cache[i].handler_idx = handler_idx & 0xFF;
			// Optimize instruction bits for popular bytecodes
			// The optimizer may also modify the bytecode if needed,
			// typically to rewrite cases where rd == zero register.
			// This avoids a check in the hot-path for rd != 0.
			const address_t pc = exec_begin + (i * sizeof(la_instruction));
			cache[i].instr = segment.optimize_bytecode(cache[i].bytecode, pc, instr);

			if (is_diverging_instruction(instr)) {
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

	uint16_t DecoderData::compute_handler_for(handler_t handler)
	{
		// Search for existing handler
		for (size_t i = 0; i < m_handlers.size(); ++i) {
			if (m_handlers[i] == handler) {
				return static_cast<uint16_t>(i);
			}
		}

		// Add new handler
		m_handlers.push_back(handler);
		return static_cast<uint16_t>(m_handlers.size() - 1);
	}

	void DecodedExecuteSegment::set(address_t entry_addr, const DecoderData& data)
	{
		const size_t index = (entry_addr - m_exec_begin) >> DecoderCache::SHIFT;
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

} // loongarch
