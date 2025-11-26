/**
 * Bytecode implementation for threaded dispatch
 * This file is included by threaded_dispatch.cpp and threaded_inaccurate_dispatch.cpp
 **/

// ============ Popular Instruction Bytecodes ============

// LA64_BC_LD_D: Load doubleword (rd = mem[rj + sign_ext(imm12)])
INSTRUCTION(LA64_BC_LD_D, la64_ld_d)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + fi.imm;
	REG(fi.rd) = MACHINE().memory.template read<uint64_t>(addr);
	NEXT_INSTR();
}

// LA64_BC_OR: Bitwise OR (rd = rj | rk)
INSTRUCTION(LA64_BC_OR, la64_or)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	REG(fi.rd) = REG(fi.rj) | REG(fi.rk);
	NEXT_INSTR();
}

// LA64_BC_ST_D: Store doubleword (mem[rj + sign_ext(imm12)] = rd)
INSTRUCTION(LA64_BC_ST_D, la64_st_d)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + fi.imm;
	MACHINE().memory.template write<uint64_t>(addr, REG(fi.rd));
	NEXT_INSTR();
}

// LA64_BC_ADDI_W: Add immediate word (rd = sign_ext((int32_t)rj + imm12))
INSTRUCTION(LA64_BC_ADDI_W, la64_addi_w)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	int32_t result = (int32_t)REG(fi.rj) + fi.imm;
	REG(fi.rd) = (int64_t)result;
	NEXT_INSTR();
}

// LA64_BC_ADDI_D: Add immediate doubleword (rd = rj + sign_ext(imm12))
INSTRUCTION(LA64_BC_ADDI_D, la64_addi_d)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	REG(fi.rd) = REG(fi.rj) + fi.imm;
	NEXT_INSTR();
}

// LA64_BC_ANDI: AND immediate (rd = rj & zero_ext(imm12))
INSTRUCTION(LA64_BC_ANDI, la64_andi)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	// ANDI uses zero-extended immediate (mask to 12 bits)
	REG(fi.rd) = REG(fi.rj) & (uint64_t)(fi.imm & 0xFFF);
	NEXT_INSTR();
}

// LA64_BC_ADD_D: Add doubleword (rd = rj + rk)
INSTRUCTION(LA64_BC_ADD_D, la64_add_d)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	REG(fi.rd) = REG(fi.rj) + REG(fi.rk);
	NEXT_INSTR();
}

// LA64_BC_SUB_D: Subtract doubleword (rd = rj - rk)
INSTRUCTION(LA64_BC_SUB_D, la64_sub_d)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	REG(fi.rd) = REG(fi.rj) - REG(fi.rk);
	NEXT_INSTR();
}

// LA64_BC_ORI: OR immediate (rd = rj | zero_ext(imm12))
INSTRUCTION(LA64_BC_ORI, la64_ori)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	// ORI uses zero-extended immediate (mask to 12 bits)
	REG(fi.rd) = REG(fi.rj) | (uint64_t)(fi.imm & 0xFFF);
	NEXT_INSTR();
}

// LA64_BC_SLLI_W: Shift left logical immediate word (rd = sign_ext((uint32_t)rj << ui5))
INSTRUCTION(LA64_BC_SLLI_W, la64_slli_w)
{
	auto fi = *(FasterLA64_Shift *)&DECODER().instr;
	uint32_t val = static_cast<uint32_t>(REG(fi.rj)) << fi.ui5;
	REG(fi.rd) = static_cast<int64_t>(static_cast<int32_t>(val));
	NEXT_INSTR();
}

// LA64_BC_SLLI_D: Shift left logical immediate doubleword (rd = rj << ui6)
INSTRUCTION(LA64_BC_SLLI_D, la64_slli_d)
{
	auto fi = *(FasterLA64_Shift64 *)&DECODER().instr;
	REG(fi.rd) = REG(fi.rj) << fi.ui6;
	NEXT_INSTR();
}

// LA64_BC_LD_BU: Load byte unsigned (rd = zero_ext(mem[rj + sign_ext(imm12)]))
INSTRUCTION(LA64_BC_LD_BU, la64_ld_bu)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + fi.imm;
	REG(fi.rd) = (uint64_t)MACHINE().memory.template read<uint8_t>(addr);
	NEXT_INSTR();
}

// LA64_BC_ST_B: Store byte (mem[rj + sign_ext(imm12)] = rd[7:0])
INSTRUCTION(LA64_BC_ST_B, la64_st_b)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + fi.imm;
	MACHINE().memory.template write<uint8_t>(addr, REG(fi.rd));
	NEXT_INSTR();
}

// LA64_BC_ST_W: Store word (mem[rj + sign_ext(imm12)] = rd[31:0])
INSTRUCTION(LA64_BC_ST_W, la64_st_w)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + fi.imm;
	MACHINE().memory.template write<uint32_t>(addr, REG(fi.rd));
	NEXT_INSTR();
}

// LA64_BC_PCADDI: PC-relative add immediate (rd = PC + sign_ext(imm20 << 2))
INSTRUCTION(LA64_BC_PCADDI, la64_pcaddi)
{
	VIEW_INSTR();
	const int32_t si20 = InstructionHelpers<W>::sign_extend_20(instr.ri20.imm);
	const int64_t offset = (si20 << 2);
	REG(instr.ri20.rd) = pc + offset;
	NEXT_BLOCK(4);
}

// LA64_BC_PCALAU12I: PC-aligned add upper immediate (rd = (PC & ~0xFFF) + (imm20 << 12))
INSTRUCTION(LA64_BC_PCALAU12I, la64_pcalau12i)
{
	VIEW_INSTR();
	const int64_t offset = (int32_t)(instr.ri20.imm << 12);
	REG(instr.ri20.rd) = (pc & ~uint64_t(0xFFF)) + offset;
	NEXT_BLOCK(4);
}

// LA64_BC_LDPTR_D: Load pointer doubleword (rd = mem[rj + sign_ext(imm14 << 2)])
INSTRUCTION(LA64_BC_LDPTR_D, la64_ldptr_d)
{
	auto fi = *(FasterLA64_RI14 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + (int64_t(fi.imm14) << 2);
	REG(fi.rd) = MACHINE().memory.template read<uint64_t>(addr);
	NEXT_INSTR();
}

// LA64_BC_LDPTR_W: Load pointer word (rd = sign_ext(mem[rj + sign_ext(imm14 << 2)]))
INSTRUCTION(LA64_BC_LDPTR_W, la64_ldptr_w)
{
	auto fi = *(FasterLA64_RI14 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + (int64_t(fi.imm14) << 2);
	REG(fi.rd) = (int64_t)(int32_t)MACHINE().memory.template read<uint32_t>(addr);
	NEXT_INSTR();
}

// LA64_BC_STPTR_D: Store pointer doubleword (mem[rj + sign_ext(imm14 << 2)] = rd)
INSTRUCTION(LA64_BC_STPTR_D, la64_stptr_d)
{
	auto fi = *(FasterLA64_RI14 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + (int64_t(fi.imm14) << 2);
	MACHINE().memory.template write<uint64_t>(addr, REG(fi.rd));
	NEXT_INSTR();
}

// LA64_BC_LU12I_W: Load upper 12-bit immediate word (rd = sign_ext(imm20 << 12))
INSTRUCTION(LA64_BC_LU12I_W, la64_lu12i_w)
{
	VIEW_INSTR();
	int32_t result = (int32_t)(instr.ri20.imm << 12);
	REG(instr.ri20.rd) = (int64_t)result;
	NEXT_INSTR();
}

// LA64_BC_BSTRPICK_D: Bit string pick doubleword (rd = extract bits[msbd:lsbd] from rj)
INSTRUCTION(LA64_BC_BSTRPICK_D, la64_bstrpick_d)
{
	auto fi = *(FasterLA64_BitField *)&DECODER().instr;
	const uint64_t src = REG(fi.rj);
	const uint32_t width = fi.msbd - fi.lsbd + 1;
	const uint64_t mask = (1ULL << width) - 1;
	REG(fi.rd) = (src >> fi.lsbd) & mask;
	NEXT_INSTR();
}

// LA64_BC_AND: Bitwise AND (rd = rj & rk)
INSTRUCTION(LA64_BC_AND, la64_and)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	REG(fi.rd) = REG(fi.rj) & REG(fi.rk);
	NEXT_INSTR();
}

// LA64_BC_ALSL_D: Arithmetic left shift and add doubleword (rd = (rj << (sa2 + 1)) + rk)
INSTRUCTION(LA64_BC_ALSL_D, la64_alsl_d)
{
	auto fi = *(FasterLA64_R3SA2 *)&DECODER().instr;
	const uint32_t shift = fi.sa2 + 1;
	REG(fi.rd) = (REG(fi.rj) << shift) + REG(fi.rk);
	NEXT_INSTR();
}

// LA64_BC_SRLI_D: Shift right logical immediate doubleword (rd = rj >> ui6)
INSTRUCTION(LA64_BC_SRLI_D, la64_srli_d)
{
	auto fi = *(FasterLA64_Shift64 *)&DECODER().instr;
	REG(fi.rd) = (uint64_t)REG(fi.rj) >> fi.ui6;
	NEXT_INSTR();
}

// LA64_BC_LD_B: Load byte signed (rd = sign_ext(mem[rj + sign_ext(imm12)]))
INSTRUCTION(LA64_BC_LD_B, la64_ld_b)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + fi.imm;
	REG(fi.rd) = (int64_t)MACHINE().memory.template read<int8_t>(addr);
	NEXT_INSTR();
}

// LA64_BC_STPTR_W: Store pointer word (mem[rj + sign_ext(imm14 << 2)] = rd[31:0])
INSTRUCTION(LA64_BC_STPTR_W, la64_stptr_w)
{
	auto fi = *(FasterLA64_RI14 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + (int64_t(fi.imm14) << 2);
	MACHINE().memory.template write<uint32_t>(addr, REG(fi.rd));
	NEXT_INSTR();
}

// LA64_BC_LDX_D: Load doubleword indexed (rd = mem[rj + rk])
INSTRUCTION(LA64_BC_LDX_D, la64_ldx_d)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + REG(fi.rk);
	REG(fi.rd) = MACHINE().memory.template read<int64_t>(addr);
	NEXT_INSTR();
}

// ============ Branching Bytecode Handlers ============

// LA64_BC_B: Unconditional branch
INSTRUCTION(LA64_BC_B, la64_b)
{
	VIEW_INSTR();
	auto offset = InstructionHelpers<W>::sign_extend_26(instr.i26.offs()) << 2;
	PERFORM_BRANCH(offset);
}

// LA64_BC_BL: Branch and link
INSTRUCTION(LA64_BC_BL, la64_bl)
{
	VIEW_INSTR();
	REG(REG_RA) = pc + 4;
	auto offset = InstructionHelpers<W>::sign_extend_26(instr.i26.offs()) << 2;
	PERFORM_BRANCH(offset);
}

// LA64_BC_BEQZ: Branch if equal to zero
INSTRUCTION(LA64_BC_BEQZ, la64_beqz)
{
	VIEW_INSTR();
	if (REG(instr.ri12.rj) == 0) {
		auto offset = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm) << 2;
		PERFORM_BRANCH(offset);
	}
	NEXT_BLOCK(4);
}

// LA64_BC_BNEZ: Branch if not equal to zero
INSTRUCTION(LA64_BC_BNEZ, la64_bnez)
{
	VIEW_INSTR();
	if (REG(instr.ri12.rj) != 0) {
		auto offset = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm) << 2;
		PERFORM_BRANCH(offset);
	}
	NEXT_BLOCK(4);
}

// LA64_BC_BEQ: Branch if equal
INSTRUCTION(LA64_BC_BEQ, la64_beq)
{
	VIEW_INSTR();
	if (REG(instr.ri16.rj) == REG(instr.ri16.rd)) {
		auto offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
		PERFORM_BRANCH(offset);
	}
	NEXT_BLOCK(4);
}

// LA64_BC_BNE: Branch if not equal
INSTRUCTION(LA64_BC_BNE, la64_bne)
{
	VIEW_INSTR();
	if (REG(instr.ri16.rj) != REG(instr.ri16.rd)) {
		auto offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
		PERFORM_BRANCH(offset);
	}
	NEXT_BLOCK(4);
}

// LA64_BC_BLT: Branch if less than
INSTRUCTION(LA64_BC_BLT, la64_blt)
{
	VIEW_INSTR();
	if ((int64_t)REG(instr.ri16.rj) < (int64_t)REG(instr.ri16.rd)) {
		auto offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
		PERFORM_BRANCH(offset);
	}
	NEXT_BLOCK(4);
}

// LA64_BC_BGE: Branch if greater than or equal
INSTRUCTION(LA64_BC_BGE, la64_bge)
{
	VIEW_INSTR();
	if ((int64_t)REG(instr.ri16.rj) >= (int64_t)REG(instr.ri16.rd)) {
		auto offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
		PERFORM_BRANCH(offset);
	}
	NEXT_BLOCK(4);
}

// LA64_BC_BLTU: Branch if less than unsigned
INSTRUCTION(LA64_BC_BLTU, la64_bltu)
{
	VIEW_INSTR();
	if ((uint64_t)REG(instr.ri16.rj) < (uint64_t)REG(instr.ri16.rd)) {
		auto offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
		PERFORM_BRANCH(offset);
	}
	NEXT_BLOCK(4);
}

// LA64_BC_BGEU: Branch if greater than or equal unsigned
INSTRUCTION(LA64_BC_BGEU, la64_bgeu)
{
	VIEW_INSTR();
	if ((uint64_t)REG(instr.ri16.rj) >= (uint64_t)REG(instr.ri16.rd)) {
		auto offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
		PERFORM_BRANCH(offset);
	}
	NEXT_BLOCK(4);
}

// LA64_BC_JIRL: Jump indirect and link register
INSTRUCTION(LA64_BC_JIRL, la64_jirl)
{
	VIEW_INSTR();
	auto next_pc = pc + 4;
	auto base = REG(instr.ri16.rj);
	auto offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
	auto target = base + offset;

	if (instr.ri16.rd != 0) {
		REG(instr.ri16.rd) = next_pc;
	}
	NEXT_BLOCK(target - pc);
}

// ============ Generic Bytecode Handlers ============

// LA64_BC_FUNCTION: Regular non-PC-modifying instruction
INSTRUCTION(LA64_BC_FUNCTION, execute_decoded_function)
{
	// Call the cached handler for this instruction
	const auto handler = DECODER().get_handler();
	handler(CPU(), la_instruction{DECODER().instr});
	NEXT_INSTR();
}

// LA64_BC_FUNCBLOCK: PC-modifying instruction (branches, jumps, PC-relative)
INSTRUCTION(LA64_BC_FUNCBLOCK, execute_function_block)
{
	VIEW_INSTR();
	REGISTERS().pc = pc;
	const auto handler = DECODER().get_handler();
	handler(CPU(), instr);
	pc = REGISTERS().pc;
	NEXT_BLOCK(4);
}

// LA64_BC_SYSCALL: System call
INSTRUCTION(LA64_BC_SYSCALL, la64_syscall)
{
	// Save PC for syscall handler
	REGISTERS().pc = pc;
	// Save instruction counter
	MACHINE().set_instruction_counter(counter);
	// Execute the system call (syscall number is in REG_A7)
	MACHINE().system_call(REG(REG_A7));
	// Restore counters
	counter = MACHINE().instruction_counter();
	max_counter = MACHINE().max_instructions();

	if (LA_UNLIKELY(max_counter == 0 || pc != REGISTERS().pc))
	{
		pc = REGISTERS().pc + 4;
		goto check_jump;
	}
	// Syscall completed normally
	NEXT_BLOCK(4);
}

// LA64_BC_INVALID: Invalid instruction
INSTRUCTION(LA64_BC_INVALID, execute_invalid)
{
	REGISTERS().pc = pc;
	MACHINE().set_instruction_counter(counter);
	// Trigger invalid instruction exception
	CPU().trigger_exception(ILLEGAL_OPCODE, pc);
}

// LA64_BC_STOP: Stop execution marker
INSTRUCTION(LA64_BC_STOP, la64_stop)
{
	REGISTERS().pc = pc;
	goto stop_execution;
}
