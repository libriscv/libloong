#include "cpu.hpp"
#include "machine.hpp"
#include <cstdio>

namespace loongarch
{
	template <int W>
	std::shared_ptr<DecodedExecuteSegment<W>>& CPU<W>::empty_execute_segment() noexcept {
		static std::shared_ptr<DecodedExecuteSegment<W>> empty_shared =
			std::make_shared<DecodedExecuteSegment<W>>(0, 0, 0);
		return empty_shared;
	}

	template <int W>
	CPU<W>::CPU(Machine<W>& machine)
		: m_machine(machine), m_exec(empty_execute_segment().get())
	{
		// Don't call reset() here - memory isn't loaded yet!
		// reset() will be called by Machine after memory is initialized
	}

	template <int W>
	CPU<W>::CPU(Machine<W>& machine, const Machine<W>& other)
		: m_machine(machine), m_exec(other.cpu.m_exec)
	{
		m_regs = other.cpu.m_regs;
	}

	template <int W>
	void CPU<W>::reset()
	{
		m_regs.reset();
		m_regs.pc = machine().memory.start_address();

		// Set up stack pointer to use the arena-allocated stack
		m_regs.get(REG_SP) = machine().memory.stack_address();
	}

	template <int W>
	void CPU<W>::trigger_exception(ExceptionType type, address_t data)
	{
		const char* msg = "Unknown exception";
		switch (type) {
			case ILLEGAL_OPCODE: msg = "Illegal opcode"; break;
			case ILLEGAL_OPERATION: msg = "Illegal operation"; break;
			case PROTECTION_FAULT: msg = "Protection fault"; break;
			case EXECUTION_SPACE_PROTECTION_FAULT: msg = "Execute protection fault"; break;
			case MISALIGNED_INSTRUCTION: msg = "Misaligned instruction"; break;
			case UNIMPLEMENTED_INSTRUCTION: msg = "Unimplemented instruction"; break;
			case MACHINE_TIMEOUT: msg = "Machine timeout"; break;
			case OUT_OF_MEMORY: msg = "Out of memory"; break;
			case INVALID_PROGRAM: msg = "Invalid program"; break;
			case FEATURE_DISABLED: msg = "Feature disabled"; break;
			case UNIMPLEMENTED_SYSCALL: msg = "Unimplemented syscall"; break;
			case GUEST_ABORT: msg = "Guest abort"; break;
		}
		throw MachineException(type, msg, data);
	}

	template <int W>
	typename CPU<W>::format_t CPU<W>::read_next_instruction() const
	{
		return memory().template read<uint32_t>(pc());
	}

	template <int W>
	void CPU<W>::execute(format_t instr)
	{
		const auto& handler = decode(instr);
		handler.handler(*this, instr);
	}

	template <int W>
	bool CPU<W>::is_executable(address_t addr) const noexcept
	{
		auto segment = memory().exec_segment_for(addr);
		return segment != nullptr;
	}

	template <int W>
	DecodedExecuteSegment<W>& CPU<W>::init_execute_area(
		const void* data, address_t begin, address_t length)
	{
		auto& segment = machine().memory.create_execute_segment(
			machine().options(), data, begin, length, true);
		this->m_exec = &segment;
		return segment;
	}

	template <int W>
	typename CPU<W>::NextExecuteReturn CPU<W>::next_execute_segment(address_t pc)
	{
		auto segment = machine().memory.exec_segment_for(pc);
		this->m_exec = segment.get();
		return {this->m_exec, pc};
	}

	template <int W>
	std::string CPU<W>::to_string(format_t format) const
	{
		char buffer[256];
		const auto& handler = decode(format);

		// Try to use the printer if available
		if (handler.printer) {
			handler.printer(buffer, sizeof(buffer), *this, format, pc());
			return std::string(buffer);
		}

		// Fallback to hex
		snprintf(buffer, sizeof(buffer), "0x%08x", format.whole);
		return std::string(buffer);
	}

	template <int W>
	std::string CPU<W>::current_instruction_to_string() const
	{
		try {
			auto instr = read_next_instruction();
			return to_string(instr);
		} catch (...) {
			return "Invalid instruction";
		}
	}

	template <int W>
	uint32_t CPU<W>::install_ebreak_at(address_t addr)
	{
		// Read current instruction
		uint32_t old_instr = memory().template read<uint32_t>(addr);
		// Write BREAK instruction
		memory().template write<uint32_t>(addr, Opcode::BREAK);
		return old_instr;
	}

	template <int W>
	void CPU<W>::step_one(bool use_instruction_counter)
	{
		auto instr = read_next_instruction();
		execute(instr);
		increment_pc(4);
		if (use_instruction_counter) {
			machine().increment_counter(1);
		}
	}

	template <int W>
	void CPU<W>::simulate_precise()
	{
		while (machine().instruction_counter() < machine().max_instructions()) {
			step_one(true);
		}
	}

	// Template instantiations
#ifdef LA_32
	template struct CPU<LA32>;
#endif
#ifdef LA_64
	template struct CPU<LA64>;
#endif

} // namespace loongarch
