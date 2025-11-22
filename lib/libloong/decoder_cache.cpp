#include "decoded_exec_segment.hpp"
#include "cpu.hpp"
#include "la_instr.hpp"
#include <cstring>

namespace loongarch
{
	// Check if an instruction is diverging (changes control flow or needs PC)
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

		// Scan backwards to calculate block_bytes
		// This computes how many bytes until the next diverging instruction
		uint32_t accumulated_bytes = 0;
		for (size_t i = num_instructions; i-- > 0; ) {
			uint32_t instr;
			std::memcpy(&instr, code + i * 4, 4);

			cache[i].instr = instr;
			// Decode and cache the handler for fast dispatch
			const auto& decoded = CPU<W>::decode(la_instruction{instr});
			cache[i].handler = decoded.handler;

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
		cache[num_instructions].handler = nullptr;
		cache[num_instructions].block_bytes = 0;

		// Store the cache in the segment
		segment.set_decoder_cache(cache, num_instructions);
	}

	// Template instantiations
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

} // namespace loongarch
