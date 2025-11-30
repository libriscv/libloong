#include "machine.hpp"
#include "la_instr.hpp"

namespace loongarch
{
	// LA32 instruction implementations

	#define INSTRUCTION(name, handler) \
		static const CPU<LA32>::instruction_t instr32_##name { handler }

	#define DECODED_INSTR(name) instr32_##name

	// INVALID instruction
	INSTRUCTION(INVALID,
	[](CPU<LA32>& cpu, la_instruction instr) {
		(void)cpu; (void)instr;
		CPU<LA32>::trigger_exception(ILLEGAL_OPCODE, instr.whole);
	});

	// Unimplemented instruction
	INSTRUCTION(UNIMPLEMENTED,
	[](CPU<LA32>& cpu, la_instruction instr) {
		CPU<LA32>::trigger_exception(UNIMPLEMENTED_INSTRUCTION, instr.whole);
	});

	// NOP
	INSTRUCTION(NOP,
	[](CPU<LA32>& cpu, la_instruction instr) {
		(void)cpu; (void)instr;
		// Do nothing
	});

	// ADD.W rd, rj, rk
	INSTRUCTION(ADD_W,
	[](CPU<LA32>& cpu, la_instruction instr) {
		cpu.reg(instr.r3.rd) = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
	});

	// SUB.W rd, rj, rk
	INSTRUCTION(SUB_W,
	[](CPU<LA32>& cpu, la_instruction instr) {
		cpu.reg(instr.r3.rd) = cpu.reg(instr.r3.rj) - cpu.reg(instr.r3.rk);
	});

	// Load/Store instructions
	INSTRUCTION(LD_W,
	[](CPU<LA32>& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers<LA32>::sign_extend_12(instr.ri12.imm);
		cpu.reg(instr.ri12.rd) = cpu.memory().template read<int32_t>(addr);
	});

	INSTRUCTION(ST_W,
	[](CPU<LA32>& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers<LA32>::sign_extend_12(instr.ri12.imm);
		cpu.memory().template write<uint32_t>(addr, cpu.reg(instr.ri12.rd));
	});

	// SYSCALL
	INSTRUCTION(SYSCALL,
	[](CPU<LA32>& cpu, la_instruction instr) {
		(void)instr;
		const int syscall_nr = cpu.reg(REG_A7);
		cpu.machine().system_call(syscall_nr);
	});

	// Decode function
	template <>
	const CPU<LA32>::instruction_t& CPU<LA32>::decode(format_t instr)
	{
		uint32_t op6 = (instr.whole & 0xFC000000) >> 26;
		uint32_t op22 = instr.whole & 0xFFC00000;
		uint32_t op17 = instr.whole & 0xFFFF8000;

		// System instructions
		if (instr.whole == Opcode::SYSCALL) return DECODED_INSTR(SYSCALL);

		switch (op6) {
			case 0x00: // ALU operations
				if (op17 == Opcode::ADD_W) return DECODED_INSTR(ADD_W);
				if (op17 == Opcode::SUB_W) return DECODED_INSTR(SUB_W);
				break;
			case 0x0A: // ADDI.W
				if (op22 == (Opcode::ADDI_W & 0xFFC00000)) return DECODED_INSTR(UNIMPLEMENTED);
				break;
			case 0x0C: // LD.W / ST.W
				if (op22 == (Opcode::LD_W & 0xFFC00000)) return DECODED_INSTR(LD_W);
				if (op22 == (Opcode::ST_W & 0xFFC00000)) return DECODED_INSTR(ST_W);
				break;
		}

		return DECODED_INSTR(UNIMPLEMENTED);
	}

	template <>
	const CPU<LA32>::instruction_t& CPU<LA32>::get_invalid_instruction() noexcept
	{
		return DECODED_INSTR(INVALID);
	}

	template <>
	const CPU<LA32>::instruction_t& CPU<LA32>::get_unimplemented_instruction() noexcept
	{
		return DECODED_INSTR(UNIMPLEMENTED);
	}

	template <>
	std::string CPU<LA32>::to_string(format_t format) const
	{
		char buffer[128];
		snprintf(buffer, sizeof(buffer), "0x%08x", format.whole);
		return std::string(buffer);
	}

} // namespace loongarch
