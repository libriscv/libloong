#include "cpu.hpp"
#include "machine.hpp"
#include "threaded_bytecodes.hpp"
#include "instruction_counter.hpp"

#define MUSTTAIL __attribute__((musttail))
#define MUNUSED  [[maybe_unused]]
#define DISPATCH_MODE_TAILCALL

// Instruction counter structure for tailcall dispatch
namespace loongarch {
	static constexpr bool TRACING = false;
	// Return type for tailcall dispatch
	using TcoRet = address_t;

	// Function pointer type for bytecode handlers
	using DecoderFunc =
		TcoRet(*)(DecoderData* d, DecodedExecuteSegment* exec, CPU& cpu, address_t pc, InstrCounter& counter);

	namespace {
		extern const DecoderFunc computed_opcode[BYTECODES_MAX];
	}
}

// Macro definitions for tailcall dispatch
#define INSTRUCTION(bytecode, name) \
	static \
	TcoRet name(DecoderData* d, MUNUSED DecodedExecuteSegment* exec, MUNUSED CPU& cpu, MUNUSED address_t pc, MUNUSED InstrCounter& counter)

#define DECODER()   (*d)
#define CPU()       cpu
#define REGISTERS() cpu.registers()
#define MACHINE()   cpu.machine()
#define REG(x)      cpu.reg(x)

#define VIEW_INSTR() \
	auto instr = la_instruction{d->instr};

#define EXECUTE_INSTR() \
	computed_opcode[d->get_bytecode()](d, exec, cpu, pc, counter)

#define EXECUTE_CURRENT() \
	MUSTTAIL return EXECUTE_INSTR();

#define RECONSTRUCT_PC() (pc - d->block_bytes)

#define NEXT_INSTR() \
	d += 1; \
	if constexpr (TRACING) { \
		printf("TRACE: PC=0x%lx Bytecode %s (%d) Instr=0x%08x\n", RECONSTRUCT_PC(), bytecode_name(d->get_bytecode()), d->get_bytecode(), d->instr); \
	} \
	EXECUTE_CURRENT()

#define RETURN_VALUES() pc

#define BEGIN_BLOCK() \
	pc += d->block_bytes; \
	counter.increment_counter(d->instruction_count()); \
	if constexpr (TRACING) { \
		printf("TRACE: End of block. New PC=0x%lx Counter=%lu/%lu\n", pc, counter.value(), counter.max()); \
	}

#define NEXT_BLOCK(offset) \
	if constexpr (TRACING) { \
		printf("TRACE: NEXT_BLOCK PC=0x%lx to 0x%lx (offset %ld)\n", pc, pc+offset, long(offset)); \
	} \
	pc += (offset); \
	d = exec->pc_relative_decoder_cache(pc); \
	OVERFLOW_CHECK(); \
	QUICK_EXEC_CHECK() \
	BEGIN_BLOCK() \
	EXECUTE_CURRENT()

#define NEXT_BLOCK_UNCHECKED(len) \
	pc += (len); \
	d += (len) >> DecoderCache::SHIFT; \
	BEGIN_BLOCK() \
	EXECUTE_CURRENT()

#define QUICK_EXEC_CHECK() \
	if (LA_UNLIKELY(!(pc >= exec->exec_begin() && pc < exec->exec_end()))) \
		MUSTTAIL return next_execute_segment(d, exec, cpu, pc, counter);

#define UNCHECKED_JUMP() \
	QUICK_EXEC_CHECK() \
	d = exec->pc_relative_decoder_cache(pc); \
	BEGIN_BLOCK() \
	EXECUTE_CURRENT()

#define OVERFLOW_CHECK() \
	if (LA_UNLIKELY(counter.overflowed())) \
		return RETURN_VALUES();

#define PERFORM_BRANCH(offset) \
	pc += (offset); \
	d += (offset) >> DecoderCache::SHIFT; \
	if constexpr (TRACING) { \
		printf("TRACE: Branch taken. New PC=0x%lx\n", pc); \
	} \
	OVERFLOW_CHECK() \
	BEGIN_BLOCK() \
	EXECUTE_CURRENT()

#define OVERFLOW_CHECKED_JUMP() \
	OVERFLOW_CHECK(); \
	UNCHECKED_JUMP();

namespace loongarch
{
	static inline DecodedExecuteSegment* resolve_execute_segment(CPU& cpu, address_t& pc)
	{
		auto results = cpu.next_execute_segment(pc);
		pc = results.pc;
		return results.exec;
	}

	// Forward declaration for next_execute_segment helper
	INSTRUCTION(0, next_execute_segment);

	// Include bytecode implementations
	#include "bytecode_impl.cpp"

	// Special bytecode handlers
	INSTRUCTION(LA64_BC_STOP, la64_stop)
	{
		(void) d;
		pc += 4; // Complete STOP instruction
		counter.stop();
		return RETURN_VALUES();
	}

	INSTRUCTION(LA64_BC_SYSCALL, la64_syscall)
	{
		// Make the current PC visible
		cpu.registers().pc = pc;
		// Make the instruction counter visible
		counter.apply(MACHINE());
		// Invoke system call
		cpu.machine().system_call(cpu.reg(REG_A7));
		// Restore counters
		counter.retrieve_counters(MACHINE());
		// System calls can change PC
		if (LA_UNLIKELY(pc != cpu.registers().pc))
		{
			pc = cpu.registers().pc;
			OVERFLOW_CHECKED_JUMP();
		}
		NEXT_BLOCK(4);
	}

	INSTRUCTION(LA64_BC_SYSCALLIMM, la64_syscall_imm)
	{
		VIEW_INSTR();
		// Make the current PC visible
		cpu.registers().pc = pc;
		counter.apply(MACHINE());
		// Execute syscall from verified immediate
		cpu.machine().system_call(d->instr);
		// Restore max counter
		counter.retrieve_counters(MACHINE());
		// Return immediately using REG_RA
		pc = REG(REG_RA);
		OVERFLOW_CHECKED_JUMP();
	}

	INSTRUCTION(0, next_execute_segment)
	{
		// Helper function to change execute segment
		exec = resolve_execute_segment(cpu, pc);
		d = exec->pc_relative_decoder_cache(pc);
		BEGIN_BLOCK();
		EXECUTE_CURRENT();
	}

#ifdef LA_BINARY_TRANSLATION
	INSTRUCTION(LA64_BC_TRANSLATOR, execute_translated_block)
	{
		// Binary translation placeholder - not yet implemented for tailcall dispatch
		// Fall back to normal execution
		cpu.trigger_exception(ILLEGAL_OPCODE);
	}
#endif

	// Bytecode function table for tailcall dispatch
	namespace {
		static const DecoderFunc computed_opcode[BYTECODES_MAX] = {
			#include "tailcall_bytecode_array.hpp"
		};
	}

	bool CPU::simulate(address_t pc, uint64_t inscounter, uint64_t maxcounter)
	{
		InstrCounter counter{inscounter, maxcounter};

		auto* exec = this->m_exec;

		// We need an execute segment matching current PC
		if (LA_UNLIKELY(!exec->is_within(pc)))
		{
			auto results = this->next_execute_segment(pc);
			exec = results.exec;
			pc = results.pc;
		}

		auto* d = exec->pc_relative_decoder_cache(pc);
		auto& cpu = *this;

		BEGIN_BLOCK();

		const address_t new_pc = EXECUTE_INSTR();

		cpu.registers().pc = new_pc;
		MACHINE().set_instruction_counter(counter.value());

		// Machine stopped normally?
		return counter.max() == 0;
	}

} // loongarch
