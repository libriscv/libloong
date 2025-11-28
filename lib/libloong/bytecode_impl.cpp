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

// LA64_BC_MOVE: Move register (rd = rk, pseudo-instruction for OR rd, zero, rk)
INSTRUCTION(LA64_BC_MOVE, la64_move)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	REG(fi.rd) = REG(fi.rk);
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
	REG(fi.rd) = (saddress_t)result;
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
	REG(fi.rd) = REG(fi.rj) & (fi.imm & 0xFFF);
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
	REG(fi.rd) = REG(fi.rj) | (fi.imm & 0xFFF);
	NEXT_INSTR();
}

// LA64_BC_SLLI_W: Shift left logical immediate word (rd = sign_ext((uint32_t)rj << ui5))
INSTRUCTION(LA64_BC_SLLI_W, la64_slli_w)
{
	auto fi = *(FasterLA64_Shift *)&DECODER().instr;
	int32_t val = static_cast<uint32_t>(REG(fi.rj)) << fi.ui5;
	REG(fi.rd) = (saddress_t)val;
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
	REG(fi.rd) = MACHINE().memory.template read<uint8_t>(addr);
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
	REG(instr.ri20.rd) = (pc & ~address_t(0xFFF)) + offset;
	NEXT_BLOCK(4);
}

// LA64_BC_LDPTR_D: Load pointer doubleword (rd = mem[rj + sign_ext(imm14 << 2)])
INSTRUCTION(LA64_BC_LDPTR_D, la64_ldptr_d)
{
	auto fi = *(FasterLA64_RI14 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + (saddress_t(fi.imm14) << 2);
	REG(fi.rd) = MACHINE().memory.template read<uint64_t>(addr);
	NEXT_INSTR();
}

// LA64_BC_LDPTR_W: Load pointer word (rd = sign_ext(mem[rj + sign_ext(imm14 << 2)]))
INSTRUCTION(LA64_BC_LDPTR_W, la64_ldptr_w)
{
	auto fi = *(FasterLA64_RI14 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + (saddress_t(fi.imm14) << 2);
	REG(fi.rd) = (saddress_t)(int32_t)MACHINE().memory.template read<uint32_t>(addr);
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
	REG(instr.ri20.rd) = (saddress_t)result;
	NEXT_INSTR();
}

// LA64_BC_BSTRPICK_D: Bit string pick doubleword (rd = extract bits[msbd:lsbd] from rj)
INSTRUCTION(LA64_BC_BSTRPICK_D, la64_bstrpick_d)
{
	auto fi = *(FasterLA64_BitField *)&DECODER().instr;
	const address_t src = REG(fi.rj);
	const uint32_t width = fi.msbd - fi.lsbd + 1;
	const address_t mask = (1ULL << width) - 1;
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
	REG(fi.rd) = REG(fi.rj) >> fi.ui6;
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
	const auto addr = REG(fi.rj) + (saddress_t(fi.imm14) << 2);
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

// LA64_BC_MASKEQZ: Mask if equal to zero (rd = (rk == 0) ? 0 : rj)
INSTRUCTION(LA64_BC_MASKEQZ, la64_maskeqz)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	REG(fi.rd) = (REG(fi.rk) == 0) ? 0 : REG(fi.rj);
	NEXT_INSTR();
}

// LA64_BC_MASKNEZ: Mask if not equal to zero (rd = (rk != 0) ? 0 : rj)
INSTRUCTION(LA64_BC_MASKNEZ, la64_masknez)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	REG(fi.rd) = (REG(fi.rk) != 0) ? 0 : REG(fi.rj);
	NEXT_INSTR();
}

// LA64_BC_MUL_D: Multiply doubleword (rd = rj * rk)
INSTRUCTION(LA64_BC_MUL_D, la64_mul_d)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	REG(fi.rd) = REG(fi.rj) * REG(fi.rk);
	NEXT_INSTR();
}

// LA64_BC_SUB_W: Subtract word (rd = sign_ext((int32_t)rj - (int32_t)rk))
INSTRUCTION(LA64_BC_SUB_W, la64_sub_w)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	int32_t result = (int32_t)REG(fi.rj) - (int32_t)REG(fi.rk);
	REG(fi.rd) = (saddress_t)result;
	NEXT_INSTR();
}

// LA64_BC_SLL_D: Shift left logical doubleword (rd = rj << (rk & 0x3F))
INSTRUCTION(LA64_BC_SLL_D, la64_sll_d)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	uint32_t shift = REG(fi.rk) & 0x3F;
	REG(fi.rd) = REG(fi.rj) << shift;
	NEXT_INSTR();
}

// LA64_BC_STX_D: Store doubleword indexed (mem[rj + rk] = rd)
INSTRUCTION(LA64_BC_STX_D, la64_stx_d)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + REG(fi.rk);
	MACHINE().memory.template write<uint64_t>(addr, REG(fi.rd));
	NEXT_INSTR();
}

// LA64_BC_BSTRPICK_W: Bit string pick word (rd = extract bits[msbw:lsbw] from rj, zero-extend)
INSTRUCTION(LA64_BC_BSTRPICK_W, la64_bstrpick_w)
{
	auto fi = *(FasterLA64_BitFieldW *)&DECODER().instr;
	const uint32_t src = (uint32_t)REG(fi.rj);
	const uint32_t width = fi.msbw - fi.lsbw + 1;
	const uint32_t mask = (1U << width) - 1;
	REG(fi.rd) = (src >> fi.lsbw) & mask;
	NEXT_INSTR();
}

// LA64_BC_SLTU: Set if less than unsigned (rd = (rj < rk) ? 1 : 0)
INSTRUCTION(LA64_BC_SLTU, la64_sltu)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	REG(fi.rd) = (REG(fi.rj) < REG(fi.rk)) ? 1 : 0;
	NEXT_INSTR();
}

// LA64_BC_LDX_W: Load word indexed (rd = sign_ext(mem[rj + rk]))
INSTRUCTION(LA64_BC_LDX_W, la64_ldx_w)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + REG(fi.rk);
	REG(fi.rd) = (saddress_t)(int32_t)MACHINE().memory.template read<int32_t>(addr);
	NEXT_INSTR();
}

// LA64_BC_STX_W: Store word indexed (mem[rj + rk] = rd[31:0])
INSTRUCTION(LA64_BC_STX_W, la64_stx_w)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + REG(fi.rk);
	MACHINE().memory.template write<uint32_t>(addr, REG(fi.rd));
	NEXT_INSTR();
}

// LA64_BC_XOR: Bitwise XOR (rd = rj ^ rk)
INSTRUCTION(LA64_BC_XOR, la64_xor)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	REG(fi.rd) = REG(fi.rj) ^ REG(fi.rk);
	NEXT_INSTR();
}

// LA64_BC_LD_HU: Load halfword unsigned (rd = zero_ext(mem[rj + sign_ext(imm12)]))
INSTRUCTION(LA64_BC_LD_HU, la64_ld_hu)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + fi.imm;
	REG(fi.rd) = MACHINE().memory.template read<uint16_t>(addr);
	NEXT_INSTR();
}

// LA64_BC_ADD_W: Add word (rd = sign_ext((int32_t)rj + (int32_t)rk))
INSTRUCTION(LA64_BC_ADD_W, la64_add_w)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	int32_t result = (int32_t)REG(fi.rj) + (int32_t)REG(fi.rk);
	REG(fi.rd) = (saddress_t)result;
	NEXT_INSTR();
}

// LA64_BC_SRAI_D: Shift right arithmetic immediate doubleword (rd = (int64_t)rj >> ui6)
INSTRUCTION(LA64_BC_SRAI_D, la64_srai_d)
{
	auto fi = *(FasterLA64_Shift64 *)&DECODER().instr;
	REG(fi.rd) = static_cast<int64_t>(REG(fi.rj)) >> fi.ui6;
	NEXT_INSTR();
}

// LA64_BC_EXT_W_B: Extend byte to word with sign (rd = sign_ext(rj[7:0]))
INSTRUCTION(LA64_BC_EXT_W_B, la64_ext_w_b)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	int8_t val = static_cast<int8_t>(REG(fi.rj));
	REG(fi.rd) = static_cast<int64_t>(val);
	NEXT_INSTR();
}

// LA64_BC_LDX_BU: Load byte unsigned indexed (rd = zero_ext(mem[rj + rk]))
INSTRUCTION(LA64_BC_LDX_BU, la64_ldx_bu)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + REG(fi.rk);
	REG(fi.rd) = (uint64_t)MACHINE().memory.template read<uint8_t>(addr);
	NEXT_INSTR();
}

// LA64_BC_BSTRINS_D: Bit string insert doubleword (rd[msbd:lsbd] = rj[msbd-lsbd:0])
INSTRUCTION(LA64_BC_BSTRINS_D, la64_bstrins_d)
{
	auto fi = *(FasterLA64_BitField *)&DECODER().instr;
	uint64_t src = REG(fi.rj);
	uint64_t dst = REG(fi.rd);

	// Valid when msbd >= lsbd
	if (fi.msbd >= fi.lsbd) {
		uint32_t width = fi.msbd - fi.lsbd + 1;
		uint64_t mask = ((1ULL << width) - 1) << fi.lsbd;
		uint64_t bits = (src << fi.lsbd) & mask;
		REG(fi.rd) = (dst & ~mask) | bits;
	}
	NEXT_INSTR();
}

// LA64_BC_LU32I_D: Load upper 32-bit immediate doubleword (rd[51:32] = si20, rd[63:52] = sign_ext(si20[19]), rd[31:0] unchanged)
INSTRUCTION(LA64_BC_LU32I_D, la64_lu32i_d)
{
	auto fi = *(FasterLA64_RI20 *)&DECODER().instr;
	uint64_t lower = REG(fi.rd) & 0xFFFFFFFF;

	// Sign-extend the 20-bit immediate to 32 bits, then place at bits [51:32]
	int32_t si20 = fi.get_imm();
	uint64_t imm_ext = ((uint64_t)(uint32_t)si20) << 32;

	REG(fi.rd) = imm_ext | lower;
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
	if ((saddress_t)REG(instr.ri16.rj) < (saddress_t)REG(instr.ri16.rd)) {
		auto offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
		PERFORM_BRANCH(offset);
	}
	NEXT_BLOCK(4);
}

// LA64_BC_BGE: Branch if greater than or equal
INSTRUCTION(LA64_BC_BGE, la64_bge)
{
	VIEW_INSTR();
	if ((saddress_t)REG(instr.ri16.rj) >= (saddress_t)REG(instr.ri16.rd)) {
		auto offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
		PERFORM_BRANCH(offset);
	}
	NEXT_BLOCK(4);
}

// LA64_BC_BLTU: Branch if less than unsigned
INSTRUCTION(LA64_BC_BLTU, la64_bltu)
{
	VIEW_INSTR();
	if (REG(instr.ri16.rj) < REG(instr.ri16.rd)) {
		auto offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
		PERFORM_BRANCH(offset);
	}
	NEXT_BLOCK(4);
}

// LA64_BC_BGEU: Branch if greater than or equal unsigned
INSTRUCTION(LA64_BC_BGEU, la64_bgeu)
{
	VIEW_INSTR();
	if (REG(instr.ri16.rj) >= REG(instr.ri16.rd)) {
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

// ============ LSX (SIMD) Instruction Bytecodes ============

// LA64_BC_VLD: Vector load 128-bit (vd = mem[rj + sign_ext(imm12)])
INSTRUCTION(LA64_BC_VLD, la64_vld)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + fi.imm;
	auto& vr = REGISTERS().getvr(fi.rd);
	vr.du[0] = MACHINE().memory.template read<uint64_t>(addr);
	vr.du[1] = MACHINE().memory.template read<uint64_t>(addr + 8);
	// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
	vr.du[2] = 0;
	vr.du[3] = 0;
	NEXT_INSTR();
}

// LA64_BC_VST: Vector store 128-bit (mem[rj + sign_ext(imm12)] = vd)
INSTRUCTION(LA64_BC_VST, la64_vst)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + fi.imm;
	const auto& vr = REGISTERS().getvr128low(fi.rd);
	MACHINE().memory.template write<remove_cvref_t<decltype(vr)>>(addr, vr);
	NEXT_INSTR();
}

// LA64_BC_VLDX: Vector indexed load 128-bit (vd = mem[rj + rk])
INSTRUCTION(LA64_BC_VLDX, la64_vldx)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + REG(fi.rk);
	auto& vr = REGISTERS().getvr(fi.rd);
	vr.du[0] = MACHINE().memory.template read<uint64_t>(addr);
	vr.du[1] = MACHINE().memory.template read<uint64_t>(addr + 8);
	// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
	vr.du[2] = 0;
	vr.du[3] = 0;
	NEXT_INSTR();
}

// LA64_BC_VSTX: Vector indexed store 128-bit (mem[rj + rk] = vd)
INSTRUCTION(LA64_BC_VSTX, la64_vstx)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + REG(fi.rk);
	const auto& vr = REGISTERS().getvr128low(fi.rd);
	MACHINE().memory.template write<remove_cvref_t<decltype(vr)>>(addr, vr);
	NEXT_INSTR();
}

// LA64_BC_VFADD_D: Vector floating-point add double (vd[i] = vj[i] + vk[i])
INSTRUCTION(LA64_BC_VFADD_D, la64_vfadd_d)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	auto& vrd = REGISTERS().getvr(fi.rd);
	const auto& vrj = REGISTERS().getvr(fi.rj);
	const auto& vrk = REGISTERS().getvr(fi.rk);
	// VFADD.D operates on 2 double-precision elements
	vrd.df[0] = vrj.df[0] + vrk.df[0];
	vrd.df[1] = vrj.df[1] + vrk.df[1];
	// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
	vrd.du[2] = 0;
	vrd.du[3] = 0;
	NEXT_INSTR();
}

// LA64_BC_VFMADD_D: Vector fused multiply-add double
INSTRUCTION(LA64_BC_VFMADD_D, la64_vfmadd_d)
{
	// 4R-type format: vd = va + vj * vk
	auto fi = *(FasterLA64_4R *)&DECODER().instr;
	auto& dst = REGISTERS().getvr(fi.rd);
	const auto& src_j = REGISTERS().getvr(fi.rj);
	const auto& src_k = REGISTERS().getvr(fi.rk);
	const auto& src_a = REGISTERS().getvr(fi.ra);

	dst.df[0] = src_a.df[0] + src_j.df[0] * src_k.df[0];
	dst.df[1] = src_a.df[1] + src_j.df[1] * src_k.df[1];
	// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
	dst.du[2] = 0;
	dst.du[3] = 0;
	NEXT_INSTR();
}

// LA64_BC_VFNMADD_D: Vector fused negative multiply-add double
INSTRUCTION(LA64_BC_VFNMADD_D, la64_vfnmadd_d)
{
	// 4R-type format: vd = -(vj * vk) + va = va - vj * vk
	auto fi = *(FasterLA64_4R *)&DECODER().instr;
	auto& dst = REGISTERS().getvr(fi.rd);
	const auto& src_j = REGISTERS().getvr(fi.rj);
	const auto& src_k = REGISTERS().getvr(fi.rk);
	const auto& src_a = REGISTERS().getvr(fi.ra);

	dst.df[0] = src_a.df[0] - src_j.df[0] * src_k.df[0];
	dst.df[1] = src_a.df[1] - src_j.df[1] * src_k.df[1];
	// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
	dst.du[2] = 0;
	dst.du[3] = 0;
	NEXT_INSTR();
}

// LA64_BC_VHADDW_D_W: Vector horizontal add with widening (word to doubleword)
INSTRUCTION(LA64_BC_VHADDW_D_W, la64_vhaddw_d_w)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	auto& dst = REGISTERS().getvr(fi.rd);
	const auto& src1 = REGISTERS().getvr(fi.rj);
	const auto& src2 = REGISTERS().getvr(fi.rk);

	// Add adjacent pairs: vj[0]+vj[1], vk[0]+vk[1]
	const int64_t res1 = (int64_t)(int32_t)src1.w[0] + (int64_t)(int32_t)src1.w[1];
	const int64_t res2 = (int64_t)(int32_t)src2.w[0] + (int64_t)(int32_t)src2.w[1];
	dst.d[0] = res1;
	dst.d[1] = res2;
	// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
	dst.d[2] = 0;
	dst.d[3] = 0;
	NEXT_INSTR();
}

// LA64_BC_XVLD: LASX 256-bit vector load
INSTRUCTION(LA64_BC_XVLD, la64_xvld)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + fi.imm;
	auto& vr = REGISTERS().getvr(fi.rd);
	vr = MACHINE().memory.template read<remove_cvref_t<decltype(vr)>>(addr);
	NEXT_INSTR();
}

// LA64_BC_XVST: LASX 256-bit vector store
INSTRUCTION(LA64_BC_XVST, la64_xvst)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + fi.imm;
	const auto& vr = REGISTERS().getvr(fi.rd);
	MACHINE().memory.template write<remove_cvref_t<decltype(vr)>>(addr, vr);
	NEXT_INSTR();
}

// LA64_BC_XVLDX: LASX 256-bit vector indexed load (xd = mem[rj + rk])
INSTRUCTION(LA64_BC_XVLDX, la64_xvldx)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + REG(fi.rk);
	auto& vr = REGISTERS().getvr(fi.rd);
	vr = MACHINE().memory.template read<remove_cvref_t<decltype(vr)>>(addr);
	NEXT_INSTR();
}

// LA64_BC_XVSTX: LASX 256-bit vector indexed store (mem[rj + rk] = xd)
INSTRUCTION(LA64_BC_XVSTX, la64_xvstx)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + REG(fi.rk);
	const auto& vr = REGISTERS().getvr(fi.rd);
	MACHINE().memory.template write<remove_cvref_t<decltype(vr)>>(addr, vr);
	NEXT_INSTR();
}

// LA64_BC_XVFADD_D: LASX floating-point add (4x double precision)
INSTRUCTION(LA64_BC_XVFADD_D, la64_xvfadd_d)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto& vr_j = REGISTERS().getvr(fi.rj);
	const auto& vr_k = REGISTERS().getvr(fi.rk);
	auto& vr_d = REGISTERS().getvr(fi.rd);
	vr_d.df[0] = vr_j.df[0] + vr_k.df[0];
	vr_d.df[1] = vr_j.df[1] + vr_k.df[1];
	vr_d.df[2] = vr_j.df[2] + vr_k.df[2];
	vr_d.df[3] = vr_j.df[3] + vr_k.df[3];
	NEXT_INSTR();
}

// LA64_BC_XVFMUL_D: LASX floating-point multiply (4x double precision)
INSTRUCTION(LA64_BC_XVFMUL_D, la64_xvfmul_d)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto& vr_j = REGISTERS().getvr(fi.rj);
	const auto& vr_k = REGISTERS().getvr(fi.rk);
	auto& vr_d = REGISTERS().getvr(fi.rd);
	vr_d.df[0] = vr_j.df[0] * vr_k.df[0];
	vr_d.df[1] = vr_j.df[1] * vr_k.df[1];
	vr_d.df[2] = vr_j.df[2] * vr_k.df[2];
	vr_d.df[3] = vr_j.df[3] * vr_k.df[3];
	NEXT_INSTR();
}

// LA64_BC_XVFMADD_D: LASX fused multiply-add (4x double precision)
INSTRUCTION(LA64_BC_XVFMADD_D, la64_xvfmadd_d)
{
	// 4R-type format: xd = xa + xj * xk
	auto fi = *(FasterLA64_4R *)&DECODER().instr;
	const auto& vr_j = REGISTERS().getvr(fi.rj);
	const auto& vr_k = REGISTERS().getvr(fi.rk);
	const auto& vr_a = REGISTERS().getvr(fi.ra);
	auto& vr_d = REGISTERS().getvr(fi.rd);
	vr_d.df[0] = vr_a.df[0] + vr_j.df[0] * vr_k.df[0];
	vr_d.df[1] = vr_a.df[1] + vr_j.df[1] * vr_k.df[1];
	vr_d.df[2] = vr_a.df[2] + vr_j.df[2] * vr_k.df[2];
	vr_d.df[3] = vr_a.df[3] + vr_j.df[3] * vr_k.df[3];
	NEXT_INSTR();
}

// LA64_BC_XVFMSUB_D: LASX fused multiply-subtract (4x double precision)
INSTRUCTION(LA64_BC_XVFMSUB_D, la64_xvfmsub_d)
{
	// 4R-type format: xd = xa - xj * xk
	auto fi = *(FasterLA64_4R *)&DECODER().instr;
	const auto& vr_j = REGISTERS().getvr(fi.rj);
	const auto& vr_k = REGISTERS().getvr(fi.rk);
	const auto& vr_a = REGISTERS().getvr(fi.ra);
	auto& vr_d = REGISTERS().getvr(fi.rd);
	vr_d.df[0] = vr_a.df[0] - vr_j.df[0] * vr_k.df[0];
	vr_d.df[1] = vr_a.df[1] - vr_j.df[1] * vr_k.df[1];
	vr_d.df[2] = vr_a.df[2] - vr_j.df[2] * vr_k.df[2];
	vr_d.df[3] = vr_a.df[3] - vr_j.df[3] * vr_k.df[3];
	NEXT_INSTR();
}

// LA64_BC_XVFNMADD_D: LASX fused negative multiply-add (4x double precision)
INSTRUCTION(LA64_BC_XVFNMADD_D, la64_xvfnmadd_d)
{
	// 4R-type format: xd = -(xj * xk) + xa = xa - xj * xk
	auto fi = *(FasterLA64_4R *)&DECODER().instr;
	const auto& vr_j = REGISTERS().getvr(fi.rj);
	const auto& vr_k = REGISTERS().getvr(fi.rk);
	const auto& vr_a = REGISTERS().getvr(fi.ra);
	auto& vr_d = REGISTERS().getvr(fi.rd);
	vr_d.df[0] = vr_a.df[0] - vr_j.df[0] * vr_k.df[0];
	vr_d.df[1] = vr_a.df[1] - vr_j.df[1] * vr_k.df[1];
	vr_d.df[2] = vr_a.df[2] - vr_j.df[2] * vr_k.df[2];
	vr_d.df[3] = vr_a.df[3] - vr_j.df[3] * vr_k.df[3];
	NEXT_INSTR();
}

// LA64_BC_XVORI_B: LASX vector OR immediate byte
INSTRUCTION(LA64_BC_XVORI_B, la64_xvori_b)
{
	la_instruction instr{DECODER().instr};
	uint32_t xd = instr.ri8.rd;
	uint32_t xj = instr.ri8.rj;
	uint32_t imm8 = instr.ri8.imm;

	const auto& vr_j = REGISTERS().getvr(xj);
	auto& vr_d = REGISTERS().getvr(xd);
	// OR each byte with the immediate value
	uint64_t imm_broadcast = 0x0101010101010101ULL * imm8;
	vr_d.du[0] = vr_j.du[0] | imm_broadcast;
	vr_d.du[1] = vr_j.du[1] | imm_broadcast;
	vr_d.du[2] = vr_j.du[2] | imm_broadcast;
	vr_d.du[3] = vr_j.du[3] | imm_broadcast;
	NEXT_INSTR();
}

// LA64_BC_XVXORI_B: LASX vector XOR immediate byte
INSTRUCTION(LA64_BC_XVXORI_B, la64_xvxori_b)
{
	la_instruction instr{DECODER().instr};
	uint32_t xd = instr.ri8.rd;
	uint32_t xj = instr.ri8.rj;
	uint32_t imm8 = instr.ri8.imm;

	const auto& vr_j = REGISTERS().getvr(xj);
	auto& vr_d = REGISTERS().getvr(xd);
	// XOR each byte with the immediate value
	uint64_t imm_broadcast = 0x0101010101010101ULL * imm8;
	vr_d.du[0] = vr_j.du[0] ^ imm_broadcast;
	vr_d.du[1] = vr_j.du[1] ^ imm_broadcast;
	vr_d.du[2] = vr_j.du[2] ^ imm_broadcast;
	vr_d.du[3] = vr_j.du[3] ^ imm_broadcast;
	NEXT_INSTR();
}

// LA64_BC_XVILVL_D: LASX vector interleave low double-word
INSTRUCTION(LA64_BC_XVILVL_D, la64_xvilvl_d)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto& vr_j = REGISTERS().getvr(fi.rj);
	const auto& vr_k = REGISTERS().getvr(fi.rk);
	auto& vr_d = REGISTERS().getvr(fi.rd);
	// Interleave low 128-bit: dst[0] = src_k[0], dst[1] = src_j[0], dst[2] = src_k[1], dst[3] = src_j[1]
	vr_d.du[0] = vr_k.du[0];
	vr_d.du[1] = vr_j.du[0];
	vr_d.du[2] = vr_k.du[1];
	vr_d.du[3] = vr_j.du[1];
	NEXT_INSTR();
}

// LA64_BC_XVILVH_D: LASX vector interleave high double-word
INSTRUCTION(LA64_BC_XVILVH_D, la64_xvilvh_d)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto& vr_j = REGISTERS().getvr(fi.rj);
	const auto& vr_k = REGISTERS().getvr(fi.rk);
	auto& vr_d = REGISTERS().getvr(fi.rd);
	// Interleave high 128-bit: dst[0] = src_k[2], dst[1] = src_j[2], dst[2] = src_k[3], dst[3] = src_j[3]
	vr_d.du[0] = vr_k.du[2];
	vr_d.du[1] = vr_j.du[2];
	vr_d.du[2] = vr_k.du[3];
	vr_d.du[3] = vr_j.du[3];
	NEXT_INSTR();
}

// LA64_BC_XVPERMI_D: LASX vector permute double-word
INSTRUCTION(LA64_BC_XVPERMI_D, la64_xvpermi_d)
{
	uint32_t xd = DECODER().instr & 0x1F;
	uint32_t xj = (DECODER().instr >> 5) & 0x1F;
	uint32_t imm8 = (DECODER().instr >> 10) & 0xFF;

	const auto& src = REGISTERS().getvr(xj);
	auto& dst = REGISTERS().getvr(xd);

	// Extract 2-bit selectors for each element
	uint32_t sel0 = (imm8 >> 0) & 0x3;
	uint32_t sel1 = (imm8 >> 2) & 0x3;
	uint32_t sel2 = (imm8 >> 4) & 0x3;
	uint32_t sel3 = (imm8 >> 6) & 0x3;

	// Need to save source in case xd == xj
	uint64_t temp[4] = { src.du[0], src.du[1], src.du[2], src.du[3] };

	// Permute elements
	dst.du[0] = temp[sel0];
	dst.du[1] = temp[sel1];
	dst.du[2] = temp[sel2];
	dst.du[3] = temp[sel3];
	NEXT_INSTR();
}

// LA64_BC_XVPACKEV_D: LASX vector pack even double-word
INSTRUCTION(LA64_BC_XVPACKEV_D, la64_xvpackev_d)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto& vr_j = REGISTERS().getvr(fi.rj);
	const auto& vr_k = REGISTERS().getvr(fi.rk);
	auto& vr_d = REGISTERS().getvr(fi.rd);
	// Pack even elements (0 and 2) from both sources
	vr_d.du[0] = vr_j.du[0];
	vr_d.du[1] = vr_k.du[0];
	vr_d.du[2] = vr_j.du[2];
	vr_d.du[3] = vr_k.du[2];
	NEXT_INSTR();
}

// LA64_BC_XVPACKOD_D: LASX vector pack odd double-word
INSTRUCTION(LA64_BC_XVPACKOD_D, la64_xvpackod_d)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto& vr_j = REGISTERS().getvr(fi.rj);
	const auto& vr_k = REGISTERS().getvr(fi.rk);
	auto& vr_d = REGISTERS().getvr(fi.rd);
	// Pack odd elements (1 and 3) from both sources
	vr_d.du[0] = vr_j.du[1];
	vr_d.du[1] = vr_k.du[1];
	vr_d.du[2] = vr_j.du[3];
	vr_d.du[3] = vr_k.du[3];
	NEXT_INSTR();
}

// LA64_BC_XVPICKEV_D: LASX vector pick even double-word
INSTRUCTION(LA64_BC_XVPICKEV_D, la64_xvpickev_d)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto& vr_j = REGISTERS().getvr(fi.rj);
	const auto& vr_k = REGISTERS().getvr(fi.rk);
	auto& vr_d = REGISTERS().getvr(fi.rd);
	// Pick even elements: dst[0]=xj[0], dst[1]=xj[2], dst[2]=xk[0], dst[3]=xk[2]
	vr_d.du[0] = vr_j.du[0];
	vr_d.du[1] = vr_j.du[2];
	vr_d.du[2] = vr_k.du[0];
	vr_d.du[3] = vr_k.du[2];
	NEXT_INSTR();
}

// LA64_BC_FMADD_D: Fused multiply-add double precision
INSTRUCTION(LA64_BC_FMADD_D, la64_fmadd_d)
{
	// 4R-type format: fd = fa + fj * fk
	auto fi = *(FasterLA64_4R *)&DECODER().instr;
	const auto& vr_j = REGISTERS().getvr(fi.rj);
	const auto& vr_k = REGISTERS().getvr(fi.rk);
	const auto& vr_a = REGISTERS().getvr(fi.ra);
	auto& vr_d = REGISTERS().getvr(fi.rd);

	vr_d.df[0] = vr_a.df[0] + vr_j.df[0] * vr_k.df[0];
	NEXT_INSTR();
}

// LA64_BC_FLDX_D: Floating-point indexed load double
INSTRUCTION(LA64_BC_FLDX_D, la64_fldx_d)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + REG(fi.rk);
	auto& vr = REGISTERS().getvr(fi.rd);
	vr.du[0] = MACHINE().memory.template read<uint64_t>(addr);
	NEXT_INSTR();
}

// LA64_BC_FSTX_D: Floating-point indexed store double
INSTRUCTION(LA64_BC_FSTX_D, la64_fstx_d)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + REG(fi.rk);
	const auto& vr = REGISTERS().getvr(fi.rd);
	MACHINE().memory.template write<uint64_t>(addr, vr.du[0]);
	NEXT_INSTR();
}

// LA64_BC_BCEQZ: Branch if condition flag equals zero
INSTRUCTION(LA64_BC_BCEQZ, la64_bceqz)
{
	VIEW_INSTR();
	const uint32_t cj = (instr.whole >> 5) & 0x7;
	int32_t offset = ((instr.whole >> 10) & 0xFFFF) << 2;
	// Sign extend 18-bit offset
	offset = (offset << (32 - 18)) >> (32 - 18);
	if (REGISTERS().cf(cj) == 0) {
		PERFORM_BRANCH(offset);
	}
	NEXT_BLOCK(4);
}

// LA64_BC_BCNEZ: Branch if condition flag not equal to zero
INSTRUCTION(LA64_BC_BCNEZ, la64_bcnez)
{
	VIEW_INSTR();
	const uint32_t cj = (instr.whole >> 5) & 0x7;
	int32_t offset = ((instr.whole >> 10) & 0xFFFF) << 2;
	// Sign extend 18-bit offset
	offset = (offset << (32 - 18)) >> (32 - 18);
	if (REGISTERS().cf(cj) != 0) {
		PERFORM_BRANCH(offset);
	}
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

// LA64_BC_SYSCALL_IMM: Immediate system call
INSTRUCTION(LA64_BC_SYSCALLIMM, la64_syscall_imm)
{
	// Save PC for syscall handler
	REGISTERS().pc = pc;
	// Execute syscall from verified immediate
	MACHINE().unchecked_system_call(DECODER().instr);
	// Restore max counter
	max_counter = MACHINE().max_instructions();

	// Return immediately using REG_RA
	pc = REG(REG_RA);
	goto check_jump;
}

// LA64_BC_CLO_W: Count leading ones word
INSTRUCTION(LA64_BC_CLO_W, la64_clo_w)
{
	auto fi = *(FasterLA64_R2 *)&DECODER().instr;
	uint32_t val = static_cast<uint32_t>(REG(fi.rj));
	int count = 0;
	for (int i = 31; i >= 0 && (val & (1u << i)); i--) count++;
	REG(fi.rd) = count;
	NEXT_INSTR();
}

// LA64_BC_CLZ_W: Count leading zeros word
INSTRUCTION(LA64_BC_CLZ_W, la64_clz_w)
{
	auto fi = *(FasterLA64_R2 *)&DECODER().instr;
	uint32_t val = static_cast<uint32_t>(REG(fi.rj));
	REG(fi.rd) = val ? __builtin_clz(val) : 32;
	NEXT_INSTR();
}

// LA64_BC_CLZ_D: Count leading zeros doubleword
INSTRUCTION(LA64_BC_CLZ_D, la64_clz_d)
{
	auto fi = *(FasterLA64_R2 *)&DECODER().instr;
	uint64_t val = REG(fi.rj);
	REG(fi.rd) = val ? __builtin_clzll(val) : 64;
	NEXT_INSTR();
}

// LA64_BC_REVB_2H: Reverse bytes in 2 halfwords
INSTRUCTION(LA64_BC_REVB_2H, la64_revb_2h)
{
	auto fi = *(FasterLA64_R2 *)&DECODER().instr;
	uint32_t val = static_cast<uint32_t>(REG(fi.rj));
	uint32_t result = ((val & 0x00FF00FF) << 8) | ((val & 0xFF00FF00) >> 8);
	REG(fi.rd) = static_cast<int64_t>(static_cast<int32_t>(result));
	NEXT_INSTR();
}

// LA64_BC_BYTEPICK_D: Byte pick doubleword
INSTRUCTION(LA64_BC_BYTEPICK_D, la64_bytepick_d)
{
	auto fi = *(FasterLA64_R3SA3 *)&DECODER().instr;
	uint64_t rj_val = REG(fi.rj);
	uint64_t rk_val = REG(fi.rk);
	uint32_t shift = fi.sa3 * 8;
	uint64_t result;
	if (shift == 0) {
		result = rj_val;
	} else {
		result = (rk_val << (64 - shift)) | (rj_val >> shift);
	}
	REG(fi.rd) = result;
	NEXT_INSTR();
}

// LA64_BC_SLTI: Set if less than immediate
INSTRUCTION(LA64_BC_SLTI, la64_slti)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	int64_t a = static_cast<int64_t>(REG(fi.rj));
	int64_t b = fi.imm;
	REG(fi.rd) = (a < b) ? 1 : 0;
	NEXT_INSTR();
}

// LA64_BC_CLO_D: Count leading ones doubleword
INSTRUCTION(LA64_BC_CLO_D, la64_clo_d)
{
	auto fi = *(FasterLA64_R2 *)&DECODER().instr;
	uint64_t val = REG(fi.rj);
	int count = 0;
	for (int i = 63; i >= 0 && (val & (1ULL << i)); i--) count++;
	REG(fi.rd) = count;
	NEXT_INSTR();
}

// LA64_BC_ST_H: Store halfword (mem[rj + sign_ext(imm12)] = rd[15:0])
INSTRUCTION(LA64_BC_ST_H, la64_st_h)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + fi.imm;
	MACHINE().memory.template write<uint16_t>(addr, REG(fi.rd));
	NEXT_INSTR();
}

// LA64_BC_FLD_D: Floating-point load doubleword
INSTRUCTION(LA64_BC_FLD_D, la64_fld_d)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + fi.imm;
	uint64_t val = MACHINE().memory.template read<uint64_t>(addr);
	auto& vr = REGISTERS().getvr(fi.rd);
	vr.du[0] = val;
	vr.du[1] = 0;
	NEXT_INSTR();
}

// LA64_BC_FST_D: Floating-point store doubleword
INSTRUCTION(LA64_BC_FST_D, la64_fst_d)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + fi.imm;
	const auto& vr = REGISTERS().getvr(fi.rd);
	MACHINE().memory.template write<uint64_t>(addr, vr.du[0]);
	NEXT_INSTR();
}

// LA64_BC_FADD_D: Floating-point add doubleword
INSTRUCTION(LA64_BC_FADD_D, la64_fadd_d)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto& vr_j = REGISTERS().getvr(fi.rj);
	const auto& vr_k = REGISTERS().getvr(fi.rk);
	auto& vr_d = REGISTERS().getvr(fi.rd);
	vr_d.df[0] = vr_j.df[0] + vr_k.df[0];
	NEXT_INSTR();
}

// LA64_BC_FMUL_D: Floating-point multiply doubleword
INSTRUCTION(LA64_BC_FMUL_D, la64_fmul_d)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto& vr_j = REGISTERS().getvr(fi.rj);
	const auto& vr_k = REGISTERS().getvr(fi.rk);
	auto& vr_d = REGISTERS().getvr(fi.rd);
	vr_d.df[0] = vr_j.df[0] * vr_k.df[0];
	NEXT_INSTR();
}

// LA64_BC_SRLI_W: Shift right logical immediate word
INSTRUCTION(LA64_BC_SRLI_W, la64_srli_w)
{
	auto fi = *(FasterLA64_Shift *)&DECODER().instr;
	uint32_t val = static_cast<uint32_t>(REG(fi.rj)) >> fi.ui5;
	REG(fi.rd) = static_cast<int32_t>(val);
	NEXT_INSTR();
}

// LA64_BC_SRL_D: Shift right logical doubleword
INSTRUCTION(LA64_BC_SRL_D, la64_srl_d)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	uint32_t shift = REG(fi.rk) & 0x3F;
	REG(fi.rd) = static_cast<uint64_t>(REG(fi.rj)) >> shift;
	NEXT_INSTR();
}

// LA64_BC_LU52I_D: Load upper 52-bit immediate doubleword
INSTRUCTION(LA64_BC_LU52I_D, la64_lu52i_d)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	// LU52I.D: GR[rd] = {imm12, GR[rj][51:0]}
	uint64_t imm = static_cast<uint64_t>(fi.imm & 0xFFF) << 52;
	uint64_t val = REG(fi.rj) & 0x000FFFFFFFFFFFFFull;
	REG(fi.rd) = imm | val;
	NEXT_INSTR();
}

// LA64_BC_XORI: XOR immediate
INSTRUCTION(LA64_BC_XORI, la64_xori)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	REG(fi.rd) = REG(fi.rj) ^ (fi.imm & 0xFFF);
	NEXT_INSTR();
}

// LA64_BC_SLTUI: Set if less than unsigned immediate
INSTRUCTION(LA64_BC_SLTUI, la64_sltui)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	uint64_t a = REG(fi.rj);
	uint64_t b = static_cast<uint64_t>(fi.imm) & 0xFFF;
	REG(fi.rd) = (a < b) ? 1 : 0;
	NEXT_INSTR();
}

// LA64_BC_LD_H: Load halfword signed
INSTRUCTION(LA64_BC_LD_H, la64_ld_h)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + fi.imm;
	REG(fi.rd) = static_cast<int64_t>(MACHINE().memory.template read<int16_t>(addr));
	NEXT_INSTR();
}

// LA64_BC_LDX_HU: Load halfword unsigned indexed
INSTRUCTION(LA64_BC_LDX_HU, la64_ldx_hu)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + REG(fi.rk);
	REG(fi.rd) = MACHINE().memory.template read<uint16_t>(addr);
	NEXT_INSTR();
}

// LA64_BC_LD_WU: Load word unsigned
INSTRUCTION(LA64_BC_LD_WU, la64_ld_wu)
{
	auto fi = *(FasterLA64_RI12 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + fi.imm;
	REG(fi.rd) = MACHINE().memory.template read<uint32_t>(addr);
	NEXT_INSTR();
}

// LA64_BC_PCADDU12I: PC-aligned add upper 12 immediate
INSTRUCTION(LA64_BC_PCADDU12I, la64_pcaddu12i)
{
	VIEW_INSTR();
	const int64_t si20 = InstructionHelpers<W>::sign_extend_20(instr.ri20.imm);
	const int64_t offset = si20 << 12;
	REG(instr.ri20.rd) = pc + offset;
	NEXT_BLOCK(4);
}

// LA64_BC_ANDN: AND NOT
INSTRUCTION(LA64_BC_ANDN, la64_andn)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	REG(fi.rd) = REG(fi.rj) & ~REG(fi.rk);
	NEXT_INSTR();
}

// LA64_BC_STX_B: Store byte indexed
INSTRUCTION(LA64_BC_STX_B, la64_stx_b)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + REG(fi.rk);
	MACHINE().memory.template write<uint8_t>(addr, REG(fi.rd));
	NEXT_INSTR();
}

// LA64_BC_CTZ_D: Count trailing zeros doubleword
INSTRUCTION(LA64_BC_CTZ_D, la64_ctz_d)
{
	auto fi = *(FasterLA64_R2 *)&DECODER().instr;
	uint64_t val = REG(fi.rj);
	REG(fi.rd) = (val == 0) ? 64 : __builtin_ctzll(val);
	NEXT_INSTR();
}

// LA64_BC_CTO_W: Count trailing ones word
INSTRUCTION(LA64_BC_CTO_W, la64_cto_w)
{
	auto fi = *(FasterLA64_R2 *)&DECODER().instr;
	uint32_t val = static_cast<uint32_t>(REG(fi.rj));
	REG(fi.rd) = (val == 0xFFFFFFFF) ? 32 : __builtin_ctz(~val);
	NEXT_INSTR();
}

// LA64_BC_EXT_W_H: Extend halfword to word
INSTRUCTION(LA64_BC_EXT_W_H, la64_ext_w_h)
{
	auto fi = *(FasterLA64_R2 *)&DECODER().instr;
	REG(fi.rd) = static_cast<int64_t>(static_cast<int16_t>(REG(fi.rj)));
	NEXT_INSTR();
}

// LA64_BC_LDX_B: Load byte signed indexed
INSTRUCTION(LA64_BC_LDX_B, la64_ldx_b)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	const auto addr = REG(fi.rj) + REG(fi.rk);
	REG(fi.rd) = static_cast<int64_t>(MACHINE().memory.template read<int8_t>(addr));
	NEXT_INSTR();
}

// LA64_BC_SLT: Set if less than
INSTRUCTION(LA64_BC_SLT, la64_slt)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	int64_t a = static_cast<int64_t>(REG(fi.rj));
	int64_t b = static_cast<int64_t>(REG(fi.rk));
	REG(fi.rd) = (a < b) ? 1 : 0;
	NEXT_INSTR();
}

// LA64_BC_ORN: OR NOT
INSTRUCTION(LA64_BC_ORN, la64_orn)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	REG(fi.rd) = REG(fi.rj) | ~REG(fi.rk);
	NEXT_INSTR();
}

// LA64_BC_CTO_D: Count trailing ones doubleword
INSTRUCTION(LA64_BC_CTO_D, la64_cto_d)
{
	auto fi = *(FasterLA64_R2 *)&DECODER().instr;
	uint64_t val = REG(fi.rj);
	REG(fi.rd) = (val == 0xFFFFFFFFFFFFFFFFull) ? 64 : __builtin_ctzll(~val);
	NEXT_INSTR();
}

// LA64_BC_MUL_W: Multiply word
INSTRUCTION(LA64_BC_MUL_W, la64_mul_w)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	int32_t a = static_cast<int32_t>(REG(fi.rj));
	int32_t b = static_cast<int32_t>(REG(fi.rk));
	REG(fi.rd) = static_cast<int64_t>(a * b);
	NEXT_INSTR();
}

// LA64_BC_MOD_DU: Modulo doubleword unsigned
INSTRUCTION(LA64_BC_MOD_DU, la64_mod_du)
{
	auto fi = *(FasterLA64_R3 *)&DECODER().instr;
	uint64_t a = REG(fi.rj);
	uint64_t b = REG(fi.rk);
	REG(fi.rd) = (b != 0) ? (a % b) : 0;
	NEXT_INSTR();
}

// LA64_BC_REVB_4H: Reverse bytes in 4 halfwords
INSTRUCTION(LA64_BC_REVB_4H, la64_revb_4h)
{
	auto fi = *(FasterLA64_R2 *)&DECODER().instr;
	uint64_t val = REG(fi.rj);
	uint64_t result = 0;
	result |= ((val & 0x00000000000000FFull) << 8);
	result |= ((val & 0x000000000000FF00ull) >> 8);
	result |= ((val & 0x0000000000FF0000ull) << 8);
	result |= ((val & 0x00000000FF000000ull) >> 8);
	result |= ((val & 0x000000FF00000000ull) << 8);
	result |= ((val & 0x0000FF0000000000ull) >> 8);
	result |= ((val & 0x00FF000000000000ull) << 8);
	result |= ((val & 0xFF00000000000000ull) >> 8);
	REG(fi.rd) = result;
	NEXT_INSTR();
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

// LA64_BC_NOP: No operation
INSTRUCTION(LA64_BC_NOP, la64_nop)
{
	NEXT_INSTR();
}

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
