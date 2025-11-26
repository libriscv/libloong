#include "decoded_exec_segment.hpp"
#include "cpu.hpp"
#include "la_instr.hpp"
#include "threaded_bytecodes.hpp"
#include <cstring>

namespace loongarch
{
	template <int W>
	extern uint32_t optimize_bytecode(const uint8_t bytecode, uint32_t instruction_bits);

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
		// OR: op17 = 0x00054 (0x00150000)
		if (op17 == 0x00054) {
			return LA64_BC_OR;
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
		// LD.BU: op10 = 0x0A8 (0x2a000000)
		if (op10 == 0x0A8) {
			return LA64_BC_LD_BU;
		}
		// ST.B: op10 = 0x0A4 (0x29000000)
		if (op10 == 0x0A4) {
			return LA64_BC_ST_B;
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

		// Branch instructions
		if (op6 == 0x14) { // B
			return LA64_BC_B;
		}
		if (op6 == 0x15) { // BL
			return LA64_BC_BL;
		}
		if (op6 == 0x10) { // BEQZ
			return LA64_BC_BEQZ;
		}
		if (op6 == 0x11) { // BNEZ
			return LA64_BC_BNEZ;
		}
		if (op6 == 0x12) { // BEQ / BNE
			const uint32_t funct3 = (instr >> 13) & 0x7;
			if (funct3 == 0x0) {
				return LA64_BC_BEQ;
			} else if (funct3 == 0x1) {
				return LA64_BC_BNE;
			}
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
		const uint8_t* code, size_t code_size)
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
			cache[i].instr = optimize_bytecode<W>(cache[i].bytecode, instr);

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

#ifdef LA_32
	template struct DecodedExecuteSegment<LA32>;
	template struct DecoderCache<LA32>;
	template struct DecoderData<LA32>;
	template void populate_decoder_cache<LA32>(DecodedExecuteSegment<LA32>&, const uint8_t*, size_t);
#endif
#ifdef LA_64
	template struct DecodedExecuteSegment<LA64>;
	template struct DecoderCache<LA64>;
	template struct DecoderData<LA64>;
	template void populate_decoder_cache<LA64>(DecodedExecuteSegment<LA64>&, const uint8_t*, size_t);
#endif
} // loongarch
