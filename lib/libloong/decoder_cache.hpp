#pragma once
#include "common.hpp"

namespace loongarch
{
	template <int W> struct CPU;

	template <int W>
	struct DecoderData {
		using handler_t = void(*)(CPU<W>&, la_instruction);

		uint32_t instr = 0;           // The 32-bit instruction bits
		handler_t handler = nullptr;  // Cached handler function pointer
		uint16_t block_bytes = 0;     // Bytes until next diverging instruction (0 = diverges here)

		// Calculate number of instructions in this block (LoongArch = 4 bytes per instruction)
		uint8_t instruction_count() const noexcept { return block_bytes / 4; }
	};

	template <int W>
	struct DecoderCache {
		static constexpr int SHIFT = 2; // 4-byte instructions

		using address_t = address_type<W>;

		DecoderData<W>* cache = nullptr;
		size_t size = 0;
	};

} // namespace loongarch
