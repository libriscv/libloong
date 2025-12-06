#pragma once
#include "common.hpp"
#include <vector>

namespace loongarch
{
	struct CPU;

	struct DecoderData {
		using handler_t = void(*)(CPU&, la_instruction);

		uint8_t bytecode;         // Bytecode for threaded dispatch
		uint8_t handler_idx;      // Handler index (0-255)
		uint16_t block_bytes;     // Bytes until next diverging instruction (0 = diverges here)
		uint32_t instr;           // The 32-bit instruction bits

		// Calculate number of instructions in this block (LoongArch = 4 bytes per instruction)
		// This includes the current (diverging) instruction: block instructions + 1
		uint8_t instruction_count() const noexcept { return (block_bytes / 4) + 1; }

		// Bytecode accessors for threaded dispatch
		uint8_t get_bytecode() const noexcept { return bytecode; }
		void set_bytecode(uint8_t bc) noexcept { bytecode = bc; }

		// Handler accessors
		handler_t get_handler() const noexcept {
			return m_handlers[handler_idx];
		}
		handler_t get_extended_handler() const noexcept {
			return m_handlers[256u + handler_idx];
		}
		static uint16_t compute_handler_for(handler_t handler);
		static handler_t* get_handlers_array() noexcept {
			return m_handlers.data();
		}

	private:
		static inline std::vector<handler_t> m_handlers;
	};
	static_assert(sizeof(DecoderData) == 8, "DecoderData size incorrect");

	struct DecoderCache {
		static constexpr int SHIFT = 2; // 4-byte instructions
		DecoderData* cache = nullptr;
		size_t size = 0;
	};

} // namespace loongarch
