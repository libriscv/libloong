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
	goto *computed_opcode[decoder->get_bytecode()];
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
	EXECUTE_INSTR();
#define PERFORM_BRANCH(offset)           \
	NEXT_BLOCK_UNCHECKED(offset);

namespace loongarch
{
	void CPU::simulate_inaccurate(address_t pc)
	{
		constexpr bool TRACE_DISPATCH = false;
		// Include computed goto table
		#include "threaded_bytecode_array.hpp"

		DecodedExecuteSegment* exec = this->m_exec;
		address_t current_begin = exec->exec_begin();
		address_t current_end   = exec->exec_end();

		// Offset the cache pointer so we can use cache[pc >> SHIFT] directly
		DecoderData* exec_decoder = exec->decoder_cache() - (current_begin >> DecoderCache::SHIFT);
		DecoderData* decoder;

		// Inaccurate mode doesn't track counter/max_counter but needs them for compatibility
		uint64_t max_counter = UINT64_MAX;
		//machine().set_max_instructions(max_counter);

		if constexpr (TRACE_DISPATCH) {
			printf("[inaccurate_dispatch] Starting at PC=0x%lx, segment [0x%lx, 0x%lx)\n",
				(unsigned long)pc, (unsigned long)current_begin, (unsigned long)current_end);
		}

		// We need an execute segment matching current PC
		if (LA_UNLIKELY(!(pc >= current_begin && pc < current_end)))
			goto new_execute_segment;

continue_segment:
		decoder = &exec_decoder[pc >> DecoderCache::SHIFT];

		if constexpr (TRACE_DISPATCH) {
			printf("  [dispatch] PC=0x%lx bytecode=%d block_bytes=%d instr_count=%d instr=0x%08x max_instr=%lu\n",
				(unsigned long)pc, decoder->get_bytecode(), decoder->block_bytes,
				decoder->instruction_count(), decoder->instr, machine().max_instructions());
		}

		pc += decoder->block_bytes;

		goto *computed_opcode[decoder->get_bytecode()];

		/** Bytecode handlers **/
		#include "bytecode_impl.cpp"

INSTRUCTION(LA64_BC_SYSCALL, la64_syscall)
{
	REGISTERS().pc = pc;
	MACHINE().set_max_instructions(max_counter);
	MACHINE().system_call(REG(REG_A7));
	// Restore counters
	max_counter = MACHINE().max_instructions();

	if (LA_UNLIKELY(max_counter == 0 || pc != REGISTERS().pc))
	{
		pc = REGISTERS().pc + 4;
		goto check_jump;
	}
	// Syscall completed normally
	NEXT_BLOCK_UNCHECKED(4);
}

INSTRUCTION(LA64_BC_SYSCALLIMM, la64_syscall_imm)
{
	REGISTERS().pc = pc;
	MACHINE().set_max_instructions(max_counter);
	// Execute syscall from verified immediate
	MACHINE().system_call(DECODER().instr);
	// Restore max counter
	max_counter = MACHINE().max_instructions();

	// Return immediately using REG_RA
	pc = REG(REG_RA);
	goto check_jump;
}

#ifdef LA_BINARY_TRANSLATION
INSTRUCTION(LA64_BC_TRANSLATOR, execute_translated_block)
{
	// The instr field contains the index into the translator mappings array
	const auto handler = exec->build_mapping(DECODER().instr);

	// Call the binary translated function
	const bintr_block_returns result = handler(CPU(), 0, ~0ull, RECONSTRUCT_PC());
	pc = REGISTERS().pc;
	max_counter = result.max_ic;
	goto check_jump;
}
#endif

INSTRUCTION(LA64_BC_STOP, la64_stop)
{
	REGISTERS().pc = pc + 4;
	return;
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
		exec_decoder  = exec->pc_relative_decoder_cache();

		if constexpr (TRACE_DISPATCH) {
			printf("  [new_execute_segment] Segment [0x%lx, 0x%lx), max_instructions=%lu\n",
				(unsigned long)current_begin, (unsigned long)current_end, machine().max_instructions());
		}

		if (max_counter)
			goto continue_segment;
		// Fall through

stop_execution:
		if constexpr (TRACE_DISPATCH) {
			printf("  [stop_execution] Stopping at PC=0x%lx\n", (unsigned long)pc);
		}
		m_regs.pc = pc;
		if (machine().has_current_exception()) {
			const auto ex = machine().current_exception();
			machine().clear_current_exception();
			std::rethrow_exception(ex);
		}
		return;

check_jump:
		if (max_counter == 0)
			goto stop_execution;

		if (LA_UNLIKELY(!(pc >= current_begin && pc < current_end)))
			goto new_execute_segment;

		goto continue_segment;
	}

} // namespace loongarch
