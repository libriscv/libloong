#include "cpu.hpp"
#include "machine.hpp"

namespace loongarch
{
	bool CPU::simulate(address_t local_pc, uint64_t counter, uint64_t max_counter)
	{
		DecodedExecuteSegment* exec = m_exec;
		// Initialize cached segment info
		address_t exec_begin = exec->exec_begin();
		address_t exec_end = exec->exec_end();
		DecoderData* cache = exec->decoder_cache() - (exec_begin >> DecoderCache::SHIFT);

		machine().set_max_instructions(max_counter);
		while (counter < max_counter) {
			// Check if PC is in current execute segment
			if (LA_UNLIKELY(local_pc < exec_begin || local_pc >= exec_end)) {
				// Update global PC and find new execute segment
				m_regs.pc = local_pc;
				auto result = next_execute_segment(local_pc); // Never null
				exec = result.exec;
				local_pc = result.pc;

				// Cache new segment info
				exec_begin = exec->exec_begin();
				exec_end = exec->exec_end();
				cache = exec->decoder_cache() - (exec_begin >> DecoderCache::SHIFT);
			}

			// Calculate PC-relative cache index
			auto* decoder = &cache[local_pc >> DecoderCache::SHIFT];

			// Execute block of non-diverging instructions + the diverging one
			// block_bytes is the count of non-diverging bytes, +1 for diverging
			unsigned block_bytes = decoder->block_bytes;
			local_pc += block_bytes;
			const auto num_instrs = decoder->instruction_count();
			counter += num_instrs;

			// Execute all non-diverging instructions in the block
			while (block_bytes >= 16) {
				decoder[0].handler(*this, la_instruction{decoder->instr});
				decoder[1].handler(*this, la_instruction{decoder[1].instr});
				decoder[2].handler(*this, la_instruction{decoder[2].instr});
				decoder[3].handler(*this, la_instruction{decoder[3].instr});
				decoder += 4;
				block_bytes -= 16;
			}
			while (block_bytes >= 4) {
				decoder->handler(*this, la_instruction{decoder->instr});
				decoder += 1;
				block_bytes -= 4;
			}

			// Now execute the diverging instruction
			// Update global PC before execution
			m_regs.pc = local_pc;
			decoder->handler(*this, la_instruction{decoder->instr});
			// Restore counter after executing diverging instruction
			// (in case of exceptions/interrupts modifying it)
			max_counter = machine().max_instructions();
			// Read updated PC after executing diverging instruction
			local_pc = m_regs.pc + 4;
		}

		// Update global state before returning
		m_regs.pc = local_pc;
		machine().set_instruction_counter(counter);
		return max_counter == 0;
	}

	void CPU::simulate_inaccurate(address_t local_pc)
	{
		DecodedExecuteSegment* exec = m_exec;
		// Initialize cached segment info
		address_t exec_begin = exec->exec_begin();
		address_t exec_end = exec->exec_end();
		DecoderData* cache = exec->decoder_cache() - (exec_begin >> DecoderCache::SHIFT);

		machine().set_max_instructions(UINT64_MAX);
		while (machine().max_instructions()) {
			// Check if PC is in current execute segment
			if (LA_UNLIKELY(local_pc < exec_begin || local_pc >= exec_end)) {
				// Update global PC and find new execute segment
				m_regs.pc = local_pc;
				auto result = next_execute_segment(local_pc); // Never null
				exec = result.exec;
				local_pc = result.pc;

				// Cache new segment info
				exec_begin = exec->exec_begin();
				exec_end = exec->exec_end();
				cache = exec->decoder_cache() - (exec_begin >> DecoderCache::SHIFT);
			}

			// Calculate PC-relative cache index
			auto* decoder = &cache[local_pc >> DecoderCache::SHIFT];

			// Execute block of non-diverging instructions + the diverging one
			// block_bytes is the count of non-diverging bytes, +1 for diverging
			unsigned block_bytes = decoder->block_bytes;
			local_pc += block_bytes;

			// Execute all non-diverging instructions in the block
			while (block_bytes >= 16) {
				decoder[0].handler(*this, la_instruction{decoder->instr});
				decoder[1].handler(*this, la_instruction{decoder[1].instr});
				decoder[2].handler(*this, la_instruction{decoder[2].instr});
				decoder[3].handler(*this, la_instruction{decoder[3].instr});
				decoder += 4;
				block_bytes -= 16;
			}
			while (block_bytes >= 4) {
				decoder->handler(*this, la_instruction{decoder->instr});
				decoder += 1;
				block_bytes -= 4;
			}

			// Now execute the diverging instruction
			// Update global PC before execution
			m_regs.pc = local_pc;
			decoder->handler(*this, la_instruction{decoder->instr});
			// Read updated PC after executing diverging instruction
			local_pc = m_regs.pc + 4;
		}

		// Update global state before returning
		m_regs.pc = local_pc;
	}

// Removed: 	template bool CPU::simulate(address_t, uint64_t, uint64_t);
// Removed: 	template void CPU::simulate_inaccurate(address_t);
#endif

} // loongarch
