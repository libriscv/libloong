#include "cpu.hpp"
#include "machine.hpp"
#include "threaded_bytecodes.hpp"

#define DECODER()   (*decoder)
#define CPU()       (*this)
#define REGISTERS() (m_regs)
#define MACHINE()   (machine())
#define REG(x)      (reg(x))
#define RECONSTRUCT_PC() (pc - DECODER().block_bytes)
#define INSTRUCTION(bc, lbl) lbl:
#define VIEW_INSTR() auto instr = la_instruction{decoder->instr};
#define EXECUTE_INSTR() \
	goto *computed_opcode[decoder->get_bytecode()]
#define NEXT_INSTR() \
	decoder += 1; \
	EXECUTE_INSTR();
#define NEXT_BLOCK(len) \
	pc += len; \
	goto check_jump;
#define NEXT_BLOCK_UNCHECKED(len) \
	pc += len; \
	decoder += len >> DecoderCache::SHIFT; \
	pc += decoder->block_bytes; \
	counter += decoder->instruction_count(); \
	EXECUTE_INSTR();
#define PERFORM_BRANCH(offset)           \
	if (LA_LIKELY(counter < max_counter)) { \
		NEXT_BLOCK_UNCHECKED(offset);    \
	}                                    \
	pc += offset;                        \
	goto check_jump;

namespace loongarch
{
	bool CPU::simulate(address_t pc, uint64_t inscounter, uint64_t maxcounter)
	{
		constexpr bool TRACE_DISPATCH = false;  // Disable for normal execution
		memory().set_arena_base_register();
		machine().set_max_instructions(UINT64_MAX);

		// Include computed goto table
		#include "threaded_bytecode_array.hpp"

		DecodedExecuteSegment* exec = this->m_exec;
		address_t current_begin = exec->exec_begin();
		address_t current_end   = exec->exec_end();

		// Offset the cache pointer so we can use cache[pc >> SHIFT] directly
		DecoderData* exec_decoder = exec->decoder_cache() - (current_begin >> DecoderCache::SHIFT);
		DecoderData* decoder;

		uint64_t counter = inscounter;
		uint64_t max_counter = maxcounter;

		// We need an execute segment matching current PC
		if (LA_UNLIKELY(!(pc >= current_begin && pc < current_end)))
			goto new_execute_segment;

continue_segment:
		decoder = &exec_decoder[pc >> DecoderCache::SHIFT];

		pc += decoder->block_bytes;
		counter += decoder->instruction_count();

		if constexpr (TRACE_DISPATCH) {
			fprintf(stderr, "[accurate] PC=0x%lx block_bytes=%u num_instrs=%u counter=%lu->%lu max=%lu\n",
				(unsigned long)(pc - decoder->block_bytes), decoder->block_bytes, decoder->instruction_count(),
				counter, counter + decoder->instruction_count(), max_counter);
			fflush(stderr);
		}

		goto *computed_opcode[decoder->get_bytecode()];

		/** Bytecode handlers **/
		#include "bytecode_impl.cpp"

INSTRUCTION(LA64_BC_STOP, la64_stop)
{
	REGISTERS().pc = pc;
	return true;
}

		// Cleanup macros
		#undef DECODER
		#undef CPU
		#undef REGISTERS
		#undef MACHINE
		#undef REG
		#undef INSTRUCTION
		#undef VIEW_INSTR
		#undef NEXT_INSTR
		#undef NEXT_BLOCK
		#undef EXECUTE_INSTR

new_execute_segment:
		m_regs.pc = pc;
		{
			auto result = next_execute_segment(pc);
			exec = result.exec;
			pc = result.pc;
		}
		current_begin = exec->exec_begin();
		current_end   = exec->exec_end();
		// Offset the cache pointer for the new segment
		exec_decoder  = exec->decoder_cache() - (current_begin >> DecoderCache::SHIFT);

		if (counter < max_counter)
			goto continue_segment;
		// Fall through

stop_execution:
		m_regs.pc = pc;
		machine().set_instruction_counter(counter);
		return max_counter == 0;

check_jump:
		if constexpr (TRACE_DISPATCH) {
			fprintf(stderr, "[check_jump] PC=0x%lx counter=%lu max=%lu\n",
				(unsigned long)pc, counter, max_counter);
			fflush(stderr);
		}
		if (counter >= max_counter) {
			if constexpr (TRACE_DISPATCH) {
				fprintf(stderr, "[check_jump] STOPPING: counter >= max\n");
				fflush(stderr);
			}
			goto stop_execution;
		}

		if (LA_UNLIKELY(!(pc >= current_begin && pc < current_end)))
			goto new_execute_segment;

		goto continue_segment;
	}

	// Removed template instantiation

} // namespace loongarch
