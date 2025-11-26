#include "cpu.hpp"
#include "machine.hpp"
#include "threaded_bytecodes.hpp"

#define DECODER()   (*decoder)
#define CPU()       (*this)
#define REGISTERS() (m_regs)
#define MACHINE()   (machine())
#define REG(x)      (reg(x))
#define INSTRUCTION(bc, lbl) lbl:
#define VIEW_INSTR() auto instr = la_instruction{decoder->instr};
#define EXECUTE_INSTR() \
	goto *computed_opcode[decoder->get_bytecode()];
#define NEXT_INSTR() \
	decoder += 1; \
	EXECUTE_INSTR();
#define NEXT_BLOCK(len) \
	pc += len; \
	goto check_jump;
#define NEXT_BLOCK_UNCHECKED(len) \
	pc += len; \
	decoder += len >> DecoderCache<W>::SHIFT; \
	pc += decoder->block_bytes; \
	EXECUTE_INSTR();
#define PERFORM_BRANCH(offset)           \
	if (LA_LIKELY(counter < max_counter)) { \
		NEXT_BLOCK_UNCHECKED(offset);    \
	}                                    \
	pc += offset;                        \
	goto check_jump;

namespace loongarch
{
	template <int W>
	void CPU<W>::simulate_inaccurate(address_t pc)
	{
#ifndef NDEBUG
		constexpr bool TRACE_DISPATCH = false;  // Set to true to enable dispatch tracing
#else
		constexpr bool TRACE_DISPATCH = false;
#endif
		machine().set_max_instructions(UINT64_MAX);

		// Include computed goto table
		#include "threaded_bytecode_array.hpp"

		DecodedExecuteSegment<W>* exec = this->m_exec;
		address_t current_begin = exec->exec_begin();
		address_t current_end   = exec->exec_end();

		// Offset the cache pointer so we can use cache[pc >> SHIFT] directly
		DecoderData<W>* exec_decoder = exec->decoder_cache() - (current_begin >> DecoderCache<W>::SHIFT);
		DecoderData<W>* decoder;

		// Inaccurate mode doesn't track counter/max_counter but needs them for compatibility
		uint64_t counter = 0;
		uint64_t max_counter = UINT64_MAX;

		if constexpr (TRACE_DISPATCH) {
			printf("[inaccurate_dispatch] Starting at PC=0x%lx, segment [0x%lx, 0x%lx)\n",
				(unsigned long)pc, (unsigned long)current_begin, (unsigned long)current_end);
		}

		// We need an execute segment matching current PC
		if (LA_UNLIKELY(!(pc >= current_begin && pc < current_end)))
			goto new_execute_segment;

continue_segment:
		decoder = &exec_decoder[pc >> DecoderCache<W>::SHIFT];

		if constexpr (TRACE_DISPATCH) {
			printf("  [dispatch] PC=0x%lx bytecode=%d block_bytes=%d instr_count=%d instr=0x%08x max_instr=%lu\n",
				(unsigned long)pc, decoder->get_bytecode(), decoder->block_bytes,
				decoder->instruction_count(), decoder->instr, machine().max_instructions());
		}

		pc += decoder->block_bytes;

		goto *computed_opcode[decoder->get_bytecode()];

		/** Bytecode handlers **/
		#include "bytecode_impl.cpp"

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
		if constexpr (TRACE_DISPATCH) {
			printf("  [new_execute_segment] Fetching new segment for PC=0x%lx\n", (unsigned long)pc);
		}
		m_regs.pc = pc;
		{
			auto result = next_execute_segment(pc);
			exec = result.exec;
			pc = result.pc;
			if constexpr (TRACE_DISPATCH) {
				printf("  [new_execute_segment] Got segment exec=%p, new PC=0x%lx\n",
					(void*)exec, (unsigned long)pc);
			}
		}
		current_begin = exec->exec_begin();
		current_end   = exec->exec_end();
		// Offset the cache pointer for the new segment
		exec_decoder  = exec->decoder_cache() - (current_begin >> DecoderCache<W>::SHIFT);

		if constexpr (TRACE_DISPATCH) {
			printf("  [new_execute_segment] Segment [0x%lx, 0x%lx), max_instructions=%lu\n",
				(unsigned long)current_begin, (unsigned long)current_end, machine().max_instructions());
		}

		if (machine().max_instructions())
			goto continue_segment;
		// Fall through

stop_execution:
		if constexpr (TRACE_DISPATCH) {
			printf("  [stop_execution] Stopping at PC=0x%lx\n", (unsigned long)pc);
		}
		m_regs.pc = pc;
		return;

check_jump:
		if (machine().max_instructions() == 0)
			goto stop_execution;

		if (LA_UNLIKELY(!(pc >= current_begin && pc < current_end)))
			goto new_execute_segment;

		goto continue_segment;
	}

#ifdef LA_32
	template void CPU<LA32>::simulate_inaccurate(address_type<LA32>);
#endif
#ifdef LA_64
	template void CPU<LA64>::simulate_inaccurate(address_type<LA64>);
#endif

} // namespace loongarch
