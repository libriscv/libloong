#pragma once
#include "cpu.hpp"
#include "la_instr.hpp"
#include <cmath>

namespace loongarch {

struct InstrImpl {
	using cpu_t = CPU;
	using addr_t = address_t;
	using saddr_t = std::make_signed_t<addr_t>;

	// === Arithmetic Instructions ===

	static void ADD_W(cpu_t& cpu, la_instruction instr) {
		int32_t result = (int32_t)cpu.reg(instr.r3.rj) + (int32_t)cpu.reg(instr.r3.rk);
		cpu.reg(instr.r3.rd) = (int64_t)result; // Sign-extend
	}

	static void ADD_D(cpu_t& cpu, la_instruction instr) {
		cpu.reg(instr.r3.rd) = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
	}

	static void SUB_W(cpu_t& cpu, la_instruction instr) {
		int32_t result = (int32_t)cpu.reg(instr.r3.rj) - (int32_t)cpu.reg(instr.r3.rk);
		cpu.reg(instr.r3.rd) = (int64_t)result;
	}

	static void SUB_D(cpu_t& cpu, la_instruction instr) {
		cpu.reg(instr.r3.rd) = cpu.reg(instr.r3.rj) - cpu.reg(instr.r3.rk);
	}

	static void SLT(cpu_t& cpu, la_instruction instr) {
		int64_t a = static_cast<int64_t>(cpu.reg(instr.r3.rj));
		int64_t b = static_cast<int64_t>(cpu.reg(instr.r3.rk));
		cpu.reg(instr.r3.rd) = (a < b) ? 1 : 0;
	}

	static void SLTU(cpu_t& cpu, la_instruction instr) {
		uint64_t a = cpu.reg(instr.r3.rj);
		uint64_t b = cpu.reg(instr.r3.rk);
		cpu.reg(instr.r3.rd) = (a < b) ? 1 : 0;
	}

	static void ADDI_W(cpu_t& cpu, la_instruction instr) {
		int32_t result = (int32_t)cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
		cpu.reg(instr.ri12.rd) = (int64_t)result;
	}

	static void ADDI_D(cpu_t& cpu, la_instruction instr) {
		cpu.reg(instr.ri12.rd) = cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
	}

	// === Division/Modulo Instructions ===

	static void DIV_W(cpu_t& cpu, la_instruction instr) {
		int32_t a = static_cast<int32_t>(cpu.reg(instr.r3.rj));
		int32_t b = static_cast<int32_t>(cpu.reg(instr.r3.rk));
		cpu.reg(instr.r3.rd) = (b != 0) ? static_cast<int64_t>(a / b) : 0;
	}

	static void MOD_W(cpu_t& cpu, la_instruction instr) {
		int32_t a = static_cast<int32_t>(cpu.reg(instr.r3.rj));
		int32_t b = static_cast<int32_t>(cpu.reg(instr.r3.rk));
		cpu.reg(instr.r3.rd) = (b != 0) ? static_cast<int64_t>(a % b) : 0;
	}

	static void DIV_WU(cpu_t& cpu, la_instruction instr) {
		uint32_t a = static_cast<uint32_t>(cpu.reg(instr.r3.rj));
		uint32_t b = static_cast<uint32_t>(cpu.reg(instr.r3.rk));
		cpu.reg(instr.r3.rd) = (b != 0) ? static_cast<int64_t>(static_cast<int32_t>(a / b)) : 0;
	}

	static void MOD_WU(cpu_t& cpu, la_instruction instr) {
		uint32_t a = static_cast<uint32_t>(cpu.reg(instr.r3.rj));
		uint32_t b = static_cast<uint32_t>(cpu.reg(instr.r3.rk));
		cpu.reg(instr.r3.rd) = (b != 0) ? static_cast<int64_t>(static_cast<int32_t>(a % b)) : 0;
	}

	static void DIV_D(cpu_t& cpu, la_instruction instr) {
		int64_t a = static_cast<int64_t>(cpu.reg(instr.r3.rj));
		int64_t b = static_cast<int64_t>(cpu.reg(instr.r3.rk));
		cpu.reg(instr.r3.rd) = (b != 0) ? (a / b) : 0;
	}

	static void MOD_D(cpu_t& cpu, la_instruction instr) {
		int64_t a = static_cast<int64_t>(cpu.reg(instr.r3.rj));
		int64_t b = static_cast<int64_t>(cpu.reg(instr.r3.rk));
		cpu.reg(instr.r3.rd) = (b != 0) ? (a % b) : 0;
	}

	static void DIV_DU(cpu_t& cpu, la_instruction instr) {
		uint64_t a = cpu.reg(instr.r3.rj);
		uint64_t b = cpu.reg(instr.r3.rk);
		cpu.reg(instr.r3.rd) = (b != 0) ? (a / b) : 0;
	}

	static void MOD_DU(cpu_t& cpu, la_instruction instr) {
		uint64_t a = cpu.reg(instr.r3.rj);
		uint64_t b = cpu.reg(instr.r3.rk);
		cpu.reg(instr.r3.rd) = (b != 0) ? (a % b) : 0;
	}

	// === Logical Instructions ===

	static void AND(cpu_t& cpu, la_instruction instr) {
		cpu.reg(instr.r3.rd) = cpu.reg(instr.r3.rj) & cpu.reg(instr.r3.rk);
	}

	static void OR(cpu_t& cpu, la_instruction instr) {
		cpu.reg(instr.r3.rd) = cpu.reg(instr.r3.rj) | cpu.reg(instr.r3.rk);
	}

	static void XOR(cpu_t& cpu, la_instruction instr) {
		cpu.reg(instr.r3.rd) = cpu.reg(instr.r3.rj) ^ cpu.reg(instr.r3.rk);
	}

	static void NOR(cpu_t& cpu, la_instruction instr) {
		cpu.reg(instr.r3.rd) = ~(cpu.reg(instr.r3.rj) | cpu.reg(instr.r3.rk));
	}

	static void ORN(cpu_t& cpu, la_instruction instr) {
		// ORN: rd = rj | ~rk
		cpu.reg(instr.r3.rd) = cpu.reg(instr.r3.rj) | ~cpu.reg(instr.r3.rk);
	}

	static void ANDN(cpu_t& cpu, la_instruction instr) {
		// ANDN: rd = rj & ~rk
		cpu.reg(instr.r3.rd) = cpu.reg(instr.r3.rj) & ~cpu.reg(instr.r3.rk);
	}

	static void MASKEQZ(cpu_t& cpu, la_instruction instr) {
		// MASKEQZ: rd = (rk == 0) ? 0 : rj
		// "Mask if Equal to Zero" - mask (zero out) if rk is zero, else pass through rj
		cpu.reg(instr.r3.rd) = (cpu.reg(instr.r3.rk) == 0) ? 0 : cpu.reg(instr.r3.rj);
	}

	static void MASKNEZ(cpu_t& cpu, la_instruction instr) {
		// MASKNEZ: rd = (rk != 0) ? 0 : rj
		// "Mask if Not Equal to Zero" - mask (zero out) if rk is non-zero, else pass through rj
		cpu.reg(instr.r3.rd) = (cpu.reg(instr.r3.rk) != 0) ? 0 : cpu.reg(instr.r3.rj);
	}

	static void ANDI(cpu_t& cpu, la_instruction instr) {
		cpu.reg(instr.ri12.rd) = cpu.reg(instr.ri12.rj) & instr.ri12.imm;
	}

	static void ORI(cpu_t& cpu, la_instruction instr) {
		cpu.reg(instr.ri12.rd) = cpu.reg(instr.ri12.rj) | instr.ri12.imm;
	}

	static void XORI(cpu_t& cpu, la_instruction instr) {
		cpu.reg(instr.ri12.rd) = cpu.reg(instr.ri12.rj) ^ instr.ri12.imm;
	}

	// === Byte Manipulation ===

	static void BYTEPICK_D(cpu_t& cpu, la_instruction instr) {
		// BYTEPICK.D rd, rj, rk, sa3
		// Concatenates rk and rj as a 128-bit value [rk:rj]
		// Then extracts 64 bits starting at byte offset sa3
		uint32_t sa3 = (instr.whole >> 15) & 0x7;
		uint64_t rj_val = cpu.reg(instr.r3.rj);
		uint64_t rk_val = cpu.reg(instr.r3.rk);

		// Shift amount in bits = sa3 * 8
		uint32_t shift = sa3 * 8;

		// Result is (rk << (64 - shift)) | (rj >> shift)
		uint64_t result;
		if (shift == 0) {
			result = rj_val;
		} else {
			result = (rk_val << (64 - shift)) | (rj_val >> shift);
		}
		cpu.reg(instr.r3.rd) = result;
	}

	// === Shift Instructions ===

	static void SLL_W(cpu_t& cpu, la_instruction instr) {
		uint32_t shift = cpu.reg(instr.r3.rk) & 0x1F;
		int32_t result = (int32_t)cpu.reg(instr.r3.rj) << shift;
		cpu.reg(instr.r3.rd) = (int64_t)result;
	}

	static void SRL_W(cpu_t& cpu, la_instruction instr) {
		uint32_t shift = cpu.reg(instr.r3.rk) & 0x1F;
		int32_t result = (uint32_t)cpu.reg(instr.r3.rj) >> shift;
		cpu.reg(instr.r3.rd) = (int64_t)result;
	}

	static void SRA_W(cpu_t& cpu, la_instruction instr) {
		uint32_t shift = cpu.reg(instr.r3.rk) & 0x1F;
		int32_t result = (int32_t)cpu.reg(instr.r3.rj) >> shift;
		cpu.reg(instr.r3.rd) = (int64_t)result;
	}

	static void SLL_D(cpu_t& cpu, la_instruction instr) {
		uint32_t shift = cpu.reg(instr.r3.rk) & 0x3F;
		cpu.reg(instr.r3.rd) = cpu.reg(instr.r3.rj) << shift;
	}

	// Shift immediate instructions
	static void SLLI_W(cpu_t& cpu, la_instruction instr) {
		uint32_t ui5 = (instr.whole >> 10) & 0x1F;
		uint32_t val = static_cast<uint32_t>(cpu.reg(instr.r3.rj)) << ui5;
		cpu.reg(instr.r3.rd) = static_cast<int32_t>(val);
	}

	static void SLLI_D(cpu_t& cpu, la_instruction instr) {
		uint32_t ui6 = (instr.whole >> 10) & 0x3F;
		cpu.reg(instr.r3.rd) = cpu.reg(instr.r3.rj) << ui6;
	}

	static void SRLI_W(cpu_t& cpu, la_instruction instr) {
		uint32_t ui5 = (instr.whole >> 10) & 0x1F;
		uint32_t val = static_cast<uint32_t>(cpu.reg(instr.r3.rj)) >> ui5;
		cpu.reg(instr.r3.rd) = static_cast<int32_t>(val);
	}

	static void SRLI_D(cpu_t& cpu, la_instruction instr) {
		uint32_t ui6 = (instr.whole >> 10) & 0x3F;
		cpu.reg(instr.r3.rd) = static_cast<uint64_t>(cpu.reg(instr.r3.rj)) >> ui6;
	}

	static void SRAI_W(cpu_t& cpu, la_instruction instr) {
		uint32_t ui5 = (instr.whole >> 10) & 0x1F;
		int32_t val = static_cast<int32_t>(cpu.reg(instr.r3.rj)) >> ui5;
		cpu.reg(instr.r3.rd) = val;
	}

	static void SRAI_D(cpu_t& cpu, la_instruction instr) {
		uint32_t ui6 = (instr.whole >> 10) & 0x3F;
		cpu.reg(instr.r3.rd) = static_cast<int64_t>(cpu.reg(instr.r3.rj)) >> ui6;
	}

	static void ROTRI_D(cpu_t& cpu, la_instruction instr) {
		uint32_t ui6 = (instr.whole >> 10) & 0x3F;
		uint64_t val = cpu.reg(instr.r3.rj);
		if (ui6 == 0) {
			cpu.reg(instr.r3.rd) = val;
		} else {
			cpu.reg(instr.r3.rd) = (val >> ui6) | (val << (64 - ui6));
		}
	}

	static void SRL_D(cpu_t& cpu, la_instruction instr) {
		uint32_t shift = cpu.reg(instr.r3.rk) & 0x3F;
		cpu.reg(instr.r3.rd) = (uint64_t)cpu.reg(instr.r3.rj) >> shift;
	}

	static void SRA_D(cpu_t& cpu, la_instruction instr) {
		uint32_t shift = cpu.reg(instr.r3.rk) & 0x3F;
		cpu.reg(instr.r3.rd) = (int64_t)cpu.reg(instr.r3.rj) >> shift;
	}

	static void ALSL_D(cpu_t& cpu, la_instruction instr) {
		// ALSL.D: GR[rd] = (GR[rj] << (sa2 + 1)) + GR[rk]
		uint32_t shift = instr.r3sa2.sa2 + 1;
		cpu.reg(instr.r3sa2.rd) = (cpu.reg(instr.r3sa2.rj) << shift) + cpu.reg(instr.r3sa2.rk);
	}

	// === Load/Store Instructions ===

	static void LD_B(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
		if (instr.ri12.rd != 0)
			cpu.reg(instr.ri12.rd) = (int64_t)cpu.memory().template read<int8_t, true>(addr);
	}

	static void LD_H(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
		if (instr.ri12.rd != 0)
			cpu.reg(instr.ri12.rd) = (int64_t)cpu.memory().template read<int16_t, true>(addr);
	}

	static void LD_W(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
		if (instr.ri12.rd != 0)
			cpu.reg(instr.ri12.rd) = (int64_t)cpu.memory().template read<int32_t, true>(addr);
	}

	static void LD_D(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
		if (instr.ri12.rd != 0)
			cpu.reg(instr.ri12.rd) = cpu.memory().template read<int64_t, true>(addr);
	}

	static void LD_BU(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
		if (instr.ri12.rd != 0)
			cpu.reg(instr.ri12.rd) = (uint64_t)cpu.memory().template read<uint8_t, true>(addr);
	}

	static void LD_HU(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
		if (instr.ri12.rd != 0)
			cpu.reg(instr.ri12.rd) = (uint64_t)cpu.memory().template read<uint16_t, true>(addr);
	}

	static void LD_WU(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
		if (instr.ri12.rd != 0)
			cpu.reg(instr.ri12.rd) = (uint64_t)cpu.memory().template read<uint32_t, true>(addr);
	}

	static void PRELD(cpu_t&, la_instruction) {
		// PRELD (prefetch for load) is a hint instruction, implemented as no-op
	}

	static void ST_B(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
		cpu.memory().template write<uint8_t, true>(addr, cpu.reg(instr.ri12.rd));
	}

	static void ST_H(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
		cpu.memory().template write<uint16_t, true>(addr, cpu.reg(instr.ri12.rd));
	}

	static void ST_W(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
		cpu.memory().template write<uint32_t, true>(addr, cpu.reg(instr.ri12.rd));
	}

	static void ST_D(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
		cpu.memory().template write<uint64_t, true>(addr, cpu.reg(instr.ri12.rd));
	}

	static void LDPTR_W(cpu_t& cpu, la_instruction instr) {
		// LDPTR.W uses 14-bit signed offset << 2 (word-aligned)
		int64_t offset = InstructionHelpers::sign_extend_14(instr.ri14.imm) << 2;
		auto addr = cpu.reg(instr.ri14.rj) + offset;
		// Sign-extend the 32-bit value to 64 bits
		cpu.reg(instr.ri14.rd) = (int64_t)(int32_t)cpu.memory().template read<uint32_t, true>(addr);
	}

	static void LDPTR_D(cpu_t& cpu, la_instruction instr) {
		// LDPTR.D uses 14-bit signed offset << 2 (word-aligned)
		int64_t offset = InstructionHelpers::sign_extend_14(instr.ri14.imm) << 2;
		auto addr = cpu.reg(instr.ri14.rj) + offset;
		cpu.reg(instr.ri14.rd) = cpu.memory().template read<uint64_t, true>(addr);
	}

	static void STPTR_W(cpu_t& cpu, la_instruction instr) {
		// STPTR.W uses 14-bit signed offset << 2 (word-aligned)
		int64_t offset = InstructionHelpers::sign_extend_14(instr.ri14.imm) << 2;
		auto addr = cpu.reg(instr.ri14.rj) + offset;
		cpu.memory().template write<uint32_t, true>(addr, cpu.reg(instr.ri14.rd));
	}

	static void STPTR_D(cpu_t& cpu, la_instruction instr) {
		// STPTR.D uses 14-bit signed offset << 2 (word-aligned)
		int64_t offset = InstructionHelpers::sign_extend_14(instr.ri14.imm) << 2;
		auto addr = cpu.reg(instr.ri14.rj) + offset;
		cpu.memory().template write<uint64_t, true>(addr, cpu.reg(instr.ri14.rd));
	}

	// === Floating-point Load/Store Instructions ===

	static void FLD_S(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
		uint32_t val = cpu.memory().template read<uint32_t, true>(addr);
		auto& vr = cpu.registers().getvr(instr.ri12.rd);
		vr.wu[0] = val;
		vr.wu[1] = 0;
		vr.du[1] = 0;
	}

	static void FST_S(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
		const auto& vr = cpu.registers().getvr(instr.ri12.rd);
		cpu.memory().template write<uint32_t, true>(addr, vr.wu[0]);
	}

	static void FLD_D(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
		uint64_t val = cpu.memory().template read<uint64_t, true>(addr);
		auto& vr = cpu.registers().getvr(instr.ri12.rd);
		vr.du[0] = val;
		vr.du[1] = 0;
	}

	static void FST_D(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
		const auto& vr = cpu.registers().getvr(instr.ri12.rd);
		cpu.memory().template write<uint64_t, true>(addr, vr.du[0]);
	}

	// === Indexed Load/Store Instructions ===

	static void STX_B(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		cpu.memory().template write<uint8_t, true>(addr, cpu.reg(instr.r3.rd));
	}

	static void STX_H(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		cpu.memory().template write<uint16_t, true>(addr, cpu.reg(instr.r3.rd));
	}

	static void STX_W(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		cpu.memory().template write<uint32_t, true>(addr, cpu.reg(instr.r3.rd));
	}

	static void STX_D(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		cpu.memory().template write<uint64_t, true>(addr, cpu.reg(instr.r3.rd));
	}

	static void FLDX_D(cpu_t& cpu, la_instruction instr) {
		// Floating-point indexed load (double precision)
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		auto& vr = cpu.registers().getvr(instr.r3.rd);
		vr.du[0] = cpu.memory().template read<uint64_t, true>(addr);
	}

	static void FSTX_D(cpu_t& cpu, la_instruction instr) {
		// Floating-point indexed store (double precision)
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		const auto& vr = cpu.registers().getvr(instr.r3.rd);
		cpu.memory().template write<uint64_t, true>(addr, vr.du[0]);
	}

	static void VLDX(cpu_t& cpu, la_instruction instr) {
		// Vector indexed load (LSX 128-bit)
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		auto& vr = cpu.registers().getvr(instr.r3.rd);
		vr.du[0] = cpu.memory().template read<uint64_t, true>(addr);
		vr.du[1] = cpu.memory().template read<uint64_t, true>(addr + 8);
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		vr.du[2] = 0;
		vr.du[3] = 0;
	}

	static void VSTX(cpu_t& cpu, la_instruction instr) {
		// Vector indexed store (LSX 128-bit)
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		const auto& vr = cpu.registers().getvr(instr.r3.rd);
		cpu.memory().template write<uint64_t, true>(addr, vr.du[0]);
		cpu.memory().template write<uint64_t, true>(addr + 8, vr.du[1]);
	}

	// === Branch Instructions ===

	static void BEQZ(cpu_t& cpu, la_instruction instr) {
		// BEQZ uses ri21 format: rj at bits[9:5], 21-bit offset split across bits[25:10] and [4:0]
		if (cpu.reg(instr.ri21.rj) == 0) {
			auto offset = InstructionHelpers::sign_extend_21(instr.ri21.offs_lo, instr.ri21.offs_hi) << 2;
			cpu.increment_pc(offset-4);
		}
	}

	static void BNEZ(cpu_t& cpu, la_instruction instr) {
		// BNEZ uses ri21 format: rj at bits[9:5], 21-bit offset split across bits[25:10] and [4:0]
		if (cpu.reg(instr.ri21.rj) != 0) {
			auto offset = InstructionHelpers::sign_extend_21(instr.ri21.offs_lo, instr.ri21.offs_hi) << 2;
			cpu.increment_pc(offset-4);
		}
	}

	static void BEQ(cpu_t& cpu, la_instruction instr) {
		if (cpu.reg(instr.ri16.rj) == cpu.reg(instr.ri16.rd)) {
			auto offset = InstructionHelpers::sign_extend_16(instr.ri16.imm) << 2;
			cpu.increment_pc(offset-4);
		}
	}

	static void BNE(cpu_t& cpu, la_instruction instr) {
		if (cpu.reg(instr.ri16.rj) != cpu.reg(instr.ri16.rd)) {
			auto offset = InstructionHelpers::sign_extend_16(instr.ri16.imm) << 2;
			cpu.increment_pc(offset-4);
		}
	}

	static void BLT(cpu_t& cpu, la_instruction instr) {
		if ((int64_t)cpu.reg(instr.ri16.rj) < (int64_t)cpu.reg(instr.ri16.rd)) {
			auto offset = InstructionHelpers::sign_extend_16(instr.ri16.imm) << 2;
			cpu.increment_pc(offset-4);
		}
	}

	static void BGE(cpu_t& cpu, la_instruction instr) {
		if ((int64_t)cpu.reg(instr.ri16.rj) >= (int64_t)cpu.reg(instr.ri16.rd)) {
			auto offset = InstructionHelpers::sign_extend_16(instr.ri16.imm) << 2;
			cpu.increment_pc(offset-4);
		}
	}

	static void BLTU(cpu_t& cpu, la_instruction instr) {
		if ((uint64_t)cpu.reg(instr.ri16.rj) < (uint64_t)cpu.reg(instr.ri16.rd)) {
			auto offset = InstructionHelpers::sign_extend_16(instr.ri16.imm) << 2;
			cpu.increment_pc(offset-4);
		}
	}

	static void BGEU(cpu_t& cpu, la_instruction instr) {
		if ((uint64_t)cpu.reg(instr.ri16.rj) >= (uint64_t)cpu.reg(instr.ri16.rd)) {
			auto offset = InstructionHelpers::sign_extend_16(instr.ri16.imm) << 2;
			cpu.increment_pc(offset-4);
		}
	}

	static void B(cpu_t& cpu, la_instruction instr) {
		auto offset = InstructionHelpers::sign_extend_26(instr.i26.offs()) << 2;
		cpu.increment_pc(offset-4);
	}

	static void BL(cpu_t& cpu, la_instruction instr) {
		cpu.reg(REG_RA) = cpu.pc() + 4;
		auto offset = InstructionHelpers::sign_extend_26(instr.i26.offs()) << 2;
		cpu.increment_pc(offset-4);
	}

	static void JIRL(cpu_t& cpu, la_instruction instr) {
		auto next_pc = cpu.pc() + 4;
		auto base = cpu.reg(instr.ri16.rj);
		auto offset = InstructionHelpers::sign_extend_16(instr.ri16.imm) << 2;
		auto target = base + offset;

		if (instr.ri16.rd != 0) {
			cpu.reg(instr.ri16.rd) = next_pc;
		}
		cpu.registers().pc = target-4;
	}

	// === Upper Immediate Instructions ===

	static void LU12I_W(cpu_t& cpu, la_instruction instr) {
		// LU12I.W: GR[rd] = SignExtend({si20, 12'b0}, GRLEN)
		int32_t result = (int32_t)(instr.ri20.imm << 12);
		cpu.reg(instr.ri20.rd) = (saddr_t)result;
	}

	static void LU32I_D(cpu_t& cpu, la_instruction instr) {
		// LU32I.D: rd[51:32] = si20, rd[63:52] = SignExtend(si20[19]), rd[31:0] unchanged
		const uint32_t lower = cpu.reg(instr.ri20.rd);

		// Sign-extend the 20-bit immediate to 32 bits, then place at bits [51:32]
		int32_t si20 = InstructionHelpers::sign_extend_20(instr.ri20.imm);
		int64_t imm_ext = (int64_t)si20 << 32;

		cpu.reg(instr.ri20.rd) = imm_ext | lower;
	}

	static void PCADDI(cpu_t& cpu, la_instruction instr) {
		int32_t si20 = InstructionHelpers::sign_extend_20(instr.ri20.imm);
		int64_t offset = (int64_t)(si20 << 2);
		cpu.reg(instr.ri20.rd) = cpu.pc() + offset;
	}

	static void PCADDU12I(cpu_t& cpu, la_instruction instr) {
		int32_t si20 = InstructionHelpers::sign_extend_20(instr.ri20.imm);
		int64_t offset = (int64_t)(si20 << 12);
		cpu.reg(instr.ri20.rd) = cpu.pc() + offset;
	}

	static void PCALAU12I(cpu_t& cpu, la_instruction instr) {
		auto pc_aligned = cpu.pc() & ~((addr_t)0xFFF);
		int64_t offset = (int64_t)(int32_t)(instr.ri20.imm << 12);
		cpu.reg(instr.ri20.rd) = pc_aligned + offset;
	}

	static void PCADDU18I(cpu_t& cpu, la_instruction instr) {
		int32_t si20 = InstructionHelpers::sign_extend_20(instr.ri20.imm);
		int64_t offset = (int64_t)(si20 << 18);
		cpu.reg(instr.ri20.rd) = cpu.pc() + offset;
	}

	static void LU52I_D(cpu_t& cpu, la_instruction instr) {
		uint64_t base = cpu.reg(instr.ri12.rj) & 0x000FFFFFFFFFFFFF;
		uint64_t upper = static_cast<uint64_t>(instr.ri12.imm) << 52;
		cpu.reg(instr.ri12.rd) = base | upper;
	}

	// === Bit Manipulation Instructions ===

	static void BSTRINS_D(cpu_t& cpu, la_instruction instr) {
		// BSTRINS.D: Insert bit string from rj[msbd-lsbd:0] into rd[msbd:lsbd]
		uint32_t msbd = (instr.whole >> 16) & 0x3F;
		uint32_t lsbd = (instr.whole >> 10) & 0x3F;
		uint64_t src = cpu.reg(instr.ri16.rj);
		uint64_t dst = cpu.reg(instr.ri16.rd);

		// Valid when msbd >= lsbd
		if (msbd >= lsbd) {
			uint32_t width = msbd - lsbd + 1;
			uint64_t mask = ((1ULL << width) - 1) << lsbd;
			uint64_t bits = (src << lsbd) & mask;
			cpu.reg(instr.ri16.rd) = (dst & ~mask) | bits;
		}
	}

	static void BSTRPICK_D(cpu_t& cpu, la_instruction instr) {
		// Extract bits [msbd:lsbd] from rj and zero-extend to rd
		uint32_t msbd = (instr.whole >> 16) & 0x3F;
		uint32_t lsbd = (instr.whole >> 10) & 0x3F;
		uint64_t src = cpu.reg(instr.ri16.rj);

		// Extract the bit field
		uint32_t width = msbd - lsbd + 1;
		uint64_t mask = (1ULL << width) - 1;
		uint64_t result = (src >> lsbd) & mask;
		cpu.reg(instr.ri16.rd) = result;
	}

	static void BSTRINS_W(cpu_t& cpu, la_instruction instr) {
		// BSTRINS.W: Insert bit string from rj[msbw-lsbw:0] into rd[msbw:lsbw]
		// msbw is 5 bits at [20:16], lsbw is 5 bits at [14:10]
		uint32_t msbw = (instr.whole >> 16) & 0x1F;
		uint32_t lsbw = (instr.whole >> 10) & 0x1F;
		uint32_t src = (uint32_t)cpu.reg(instr.ri16.rj);
		uint32_t dst = (uint32_t)cpu.reg(instr.ri16.rd);

		// Valid when msbw >= lsbw
		if (msbw >= lsbw) {
			uint32_t width = msbw - lsbw + 1;
			uint32_t mask = ((1U << width) - 1) << lsbw;
			uint32_t bits = (src << lsbw) & mask;
			uint32_t result = (dst & ~mask) | bits;
			// Sign-extend to 64 bits
			cpu.reg(instr.ri16.rd) = (int64_t)(int32_t)result;
		}
	}

	static void BSTRPICK_W(cpu_t& cpu, la_instruction instr) {
		// Extract bits [msbw:lsbw] from rj and zero-extend to rd (32-bit version)
		// msbw is 5 bits at [20:16], lsbw is 5 bits at [14:10]
		uint32_t msbw = (instr.whole >> 16) & 0x1F;
		uint32_t lsbw = (instr.whole >> 10) & 0x1F;
		uint32_t src = (uint32_t)cpu.reg(instr.ri16.rj);

		// Extract the bit field
		uint32_t width = msbw - lsbw + 1;
		uint32_t mask = (1U << width) - 1;
		uint32_t result = (src >> lsbw) & mask;
		// Zero-extend to 64 bits (unsigned)
		cpu.reg(instr.ri16.rd) = result;
	}

	// === System Instructions ===

	static void SYSCALL(cpu_t& cpu, la_instruction instr) {
		(void)instr;
		const int syscall_nr = static_cast<uint32_t>(cpu.reg(REG_A7));
		cpu.machine().system_call(syscall_nr);
	}

	static void NOP(cpu_t& cpu, la_instruction instr) {
		(void)cpu; (void)instr;
	}

	static void RDTIME_D(cpu_t& cpu, la_instruction instr) {
		// Read time counter
		// rd = Machine::rdtime()
		cpu.reg(instr.r2.rd) = cpu.machine().rdtime();
	}

	static void CPUCFG(cpu_t& cpu, la_instruction instr) {
		// CPUCFG: rd = CPU configuration register
		// For simplicity, return a fixed value indicating a basic LoongArch CPU
		const uint64_t CPUCFG_BASIC = 0x0000000000000001;
		cpu.reg(instr.r2.rd) = CPUCFG_BASIC;
	}

	// === Memory Barrier Instructions ===

	static void DBAR(cpu_t& cpu, la_instruction instr) {
		// Memory barrier - no-op in single-threaded userspace emulator
		(void)cpu; (void)instr;
	}

	static void IBAR(cpu_t& cpu, la_instruction instr) {
		// Instruction barrier - no-op in interpreter mode
		(void)cpu; (void)instr;
	}

	// === Load-Linked / Store-Conditional ===

	static void LL_W(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri14.rj) + (InstructionHelpers::sign_extend_14(instr.ri14.imm) << 2);
		cpu.reg(instr.ri14.rd) = (int64_t)(int32_t)cpu.memory().template read<uint32_t, true>(addr);
		// In single-threaded mode, we always succeed
		cpu.set_ll_bit(true);
	}

	static void LL_D(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri14.rj) + (InstructionHelpers::sign_extend_14(instr.ri14.imm) << 2);
		cpu.reg(instr.ri14.rd) = cpu.memory().template read<uint64_t, true>(addr);
		cpu.set_ll_bit(true);
	}

	static void SC_W(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri14.rj) + (InstructionHelpers::sign_extend_14(instr.ri14.imm) << 2);
		if (cpu.ll_bit()) {
			cpu.memory().template write<uint32_t, true>(addr, cpu.reg(instr.ri14.rd));
			cpu.reg(instr.ri14.rd) = 1; // Success
		} else {
			cpu.reg(instr.ri14.rd) = 0; // Failure
		}
		cpu.set_ll_bit(false);
	}

	static void SC_D(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri14.rj) + (InstructionHelpers::sign_extend_14(instr.ri14.imm) << 2);
		if (cpu.ll_bit()) {
			cpu.memory().template write<uint64_t, true>(addr, cpu.reg(instr.ri14.rd));
			cpu.reg(instr.ri14.rd) = 1; // Success
		} else {
			cpu.reg(instr.ri14.rd) = 0; // Failure
		}
		cpu.set_ll_bit(false);
	}

	// === Indexed Load Instructions ===

	static void LDX_B(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		cpu.reg(instr.r3.rd) = (int64_t)cpu.memory().template read<int8_t, true>(addr);
	}

	static void LDX_H(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		cpu.reg(instr.r3.rd) = (int64_t)cpu.memory().template read<int16_t, true>(addr);
	}

	static void LDX_W(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		cpu.reg(instr.r3.rd) = (int64_t)cpu.memory().template read<int32_t, true>(addr);
	}

	static void LDX_D(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		cpu.reg(instr.r3.rd) = cpu.memory().template read<int64_t, true>(addr);
	}

	static void LDX_BU(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		cpu.reg(instr.r3.rd) = (uint64_t)cpu.memory().template read<uint8_t, true>(addr);
	}

	static void LDX_HU(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		cpu.reg(instr.r3.rd) = (uint64_t)cpu.memory().template read<uint16_t, true>(addr);
	}

	static void LDX_WU(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		cpu.reg(instr.r3.rd) = (uint64_t)cpu.memory().template read<uint32_t, true>(addr);
	}

	// === Multiply Instructions ===

	static void MUL_W(cpu_t& cpu, la_instruction instr) {
		int32_t a = static_cast<int32_t>(cpu.reg(instr.r3.rj));
		int32_t b = static_cast<int32_t>(cpu.reg(instr.r3.rk));
		cpu.reg(instr.r3.rd) = static_cast<int64_t>(a * b);
	}

	static void MULH_W(cpu_t& cpu, la_instruction instr) {
		int64_t a = static_cast<int32_t>(cpu.reg(instr.r3.rj));
		int64_t b = static_cast<int32_t>(cpu.reg(instr.r3.rk));
		int64_t result = (a * b) >> 32;
		cpu.reg(instr.r3.rd) = static_cast<int64_t>(static_cast<int32_t>(result));
	}

	static void MULH_WU(cpu_t& cpu, la_instruction instr) {
		uint64_t a = static_cast<uint32_t>(cpu.reg(instr.r3.rj));
		uint64_t b = static_cast<uint32_t>(cpu.reg(instr.r3.rk));
		uint64_t result = (a * b) >> 32;
		cpu.reg(instr.r3.rd) = static_cast<int64_t>(static_cast<int32_t>(result));
	}

	static void MUL_D(cpu_t& cpu, la_instruction instr) {
		cpu.reg(instr.r3.rd) = cpu.reg(instr.r3.rj) * cpu.reg(instr.r3.rk);
	}

	static void MULH_D(cpu_t& cpu, la_instruction instr) {
		__int128 a = static_cast<int64_t>(cpu.reg(instr.r3.rj));
		__int128 b = static_cast<int64_t>(cpu.reg(instr.r3.rk));
		cpu.reg(instr.r3.rd) = static_cast<int64_t>((a * b) >> 64);
	}

	static void MULH_DU(cpu_t& cpu, la_instruction instr) {
		unsigned __int128 a = cpu.reg(instr.r3.rj);
		unsigned __int128 b = cpu.reg(instr.r3.rk);
		cpu.reg(instr.r3.rd) = static_cast<uint64_t>((a * b) >> 64);
	}

	// === Comparison Immediate Instructions ===

	static void SLTI(cpu_t& cpu, la_instruction instr) {
		int64_t a = static_cast<int64_t>(cpu.reg(instr.ri12.rj));
		int64_t b = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		cpu.reg(instr.ri12.rd) = (a < b) ? 1 : 0;
	}

	static void SLTUI(cpu_t& cpu, la_instruction instr) {
		uint64_t a = cpu.reg(instr.ri12.rj);
		uint64_t b = static_cast<uint64_t>(InstructionHelpers::sign_extend_12(instr.ri12.imm));
		cpu.reg(instr.ri12.rd) = (a < b) ? 1 : 0;
	}

	// === Additional Rotate Instructions ===

	static void ROTR_W(cpu_t& cpu, la_instruction instr) {
		uint32_t val = static_cast<uint32_t>(cpu.reg(instr.r3.rj));
		uint32_t shift = cpu.reg(instr.r3.rk) & 0x1F;
		uint32_t result = (shift == 0) ? val : ((val >> shift) | (val << (32 - shift)));
		cpu.reg(instr.r3.rd) = static_cast<int64_t>(static_cast<int32_t>(result));
	}

	static void ROTR_D(cpu_t& cpu, la_instruction instr) {
		uint64_t val = cpu.reg(instr.r3.rj);
		uint32_t shift = cpu.reg(instr.r3.rk) & 0x3F;
		cpu.reg(instr.r3.rd) = (shift == 0) ? val : ((val >> shift) | (val << (64 - shift)));
	}

	static void ROTRI_W(cpu_t& cpu, la_instruction instr) {
		uint32_t ui5 = (instr.whole >> 10) & 0x1F;
		uint32_t val = static_cast<uint32_t>(cpu.reg(instr.r3.rj));
		uint32_t result = (ui5 == 0) ? val : ((val >> ui5) | (val << (32 - ui5)));
		cpu.reg(instr.r3.rd) = static_cast<int64_t>(static_cast<int32_t>(result));
	}

	// === Bit Manipulation Instructions ===

	static void EXT_W_B(cpu_t& cpu, la_instruction instr) {
		// Sign-extend byte to word
		int8_t val = static_cast<int8_t>(cpu.reg(instr.r2.rj));
		cpu.reg(instr.r2.rd) = static_cast<int64_t>(val);
	}

	static void EXT_W_H(cpu_t& cpu, la_instruction instr) {
		// Sign-extend halfword to word
		int16_t val = static_cast<int16_t>(cpu.reg(instr.r2.rj));
		cpu.reg(instr.r2.rd) = static_cast<int64_t>(val);
	}

	static void MOVFR2GR_S(cpu_t& cpu, la_instruction instr) {
		// Move 32-bit float from FPR to GPR (sign-extended)
		uint32_t rd = instr.whole & 0x1F;
		if (rd == 0) return; // Writes to x0 are discarded
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		// In LoongArch, FP registers share storage with LSX vector registers
		// $fa0 is the low 64 bits of $vr0, so we read from the vector register
		const auto& vr = cpu.registers().getvr(fj);
		int32_t val = static_cast<int32_t>(vr.wu[0]);  // Low 32 bits
		// Sign-extend to 64 bits
		cpu.reg(rd) = static_cast<int64_t>(val);
	}

	static void MOVFR2GR_D(cpu_t& cpu, la_instruction instr) {
		// Move 64-bit value from FPR to GPR
		uint32_t rd = instr.whole & 0x1F;
		if (rd == 0) return; // Writes to x0 are discarded
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		// In LoongArch, FP registers share storage with LSX vector registers
		// $fa0 is the low 64 bits of $vr0, so we read from the vector register
		const auto& vr = cpu.registers().getvr(fj);
		cpu.reg(rd) = vr.du[0];  // Read 64-bit value from low doubleword
	}

	static void MOVGR2FR_W(cpu_t& cpu, la_instruction instr) {
		// Move 32-bit value from GPR to FPR (word)
		uint32_t fd = instr.whole & 0x1F;
		uint32_t rj = (instr.whole >> 5) & 0x1F;
		// In LoongArch, FP registers share storage with LSX vector registers
		// $fa0 is the low 64 bits of $vr0, so we write to the vector register
		// For .w variant, write 32-bit value to low word and sign-extend to 64 bits
		auto& vr = cpu.registers().getvr(fd);
		uint32_t value = cpu.reg(rj) & 0xFFFFFFFF;
		vr.wu[0] = value;  // Write 32-bit value to low word
		vr.wu[1] = 0;      // Clear upper 32 bits of low doubleword
	}

	static void MOVGR2FR_D(cpu_t& cpu, la_instruction instr) {
		// Move 64-bit value from GPR to FPR
		uint32_t fd = instr.whole & 0x1F;
		uint32_t rj = (instr.whole >> 5) & 0x1F;
		// In LoongArch, FP registers share storage with LSX vector registers
		// $fa0 is the low 64 bits of $vr0, so we write to the vector register
		auto& vr = cpu.registers().getvr(fd);
		vr.du[0] = cpu.reg(rj);  // Write 64-bit value to low doubleword
	}

	static void MOVFCSR2GR(cpu_t& cpu, la_instruction instr) {
		// Move FCSR (floating-point control/status register) to GPR
		uint32_t rd = instr.whole & 0x1F;
		if (rd == 0) return; // Writes to x0 are discarded
		// Note: fcsr index is in bits [9:5] but for FCSR0 it's always 0
		cpu.reg(rd) = cpu.registers().fcsr();
	}

	static void MOVFR2CF(cpu_t& cpu, la_instruction instr) {
		// Move lowest bit of FPR to condition flag
		// Format: movfr2cf cd, fj
		uint32_t cd = instr.whole & 0x7;         // FCC register index (3 bits)
		uint32_t fj = (instr.whole >> 5) & 0x1F; // Source FP register
		const auto& vr = cpu.registers().getvr(fj);
		uint8_t bit = vr.du[0] & 1;  // Get lowest bit
		cpu.registers().set_cf(cd, bit);
	}

	static void MOVCF2FR(cpu_t& cpu, la_instruction instr) {
		// Move condition flag to lowest bit of FPR
		// Format: movcf2fr fd, cj
		uint32_t fd = instr.whole & 0x1F;        // Destination FP register
		uint32_t cj = (instr.whole >> 5) & 0x7;  // Source FCC register (3 bits)
		auto& vr = cpu.registers().getvr(fd);
		vr.du[0] = cpu.registers().cf(cj);  // Write condition flag to lowest bit
	}

	static void MOVGR2CF(cpu_t& cpu, la_instruction instr) {
		// Move lowest bit of GPR to condition flag
		// Format: movgr2cf cd, rj
		uint32_t cd = instr.whole & 0x7;         // FCC register index (3 bits)
		uint32_t rj = (instr.whole >> 5) & 0x1F; // Source general register
		uint8_t bit = cpu.reg(rj) & 1;  // Get lowest bit
		cpu.registers().set_cf(cd, bit);
	}

	static void MOVCF2GR(cpu_t& cpu, la_instruction instr) {
		// Move condition flag to lowest bit of GPR, clear other bits
		// Format: movcf2gr rd, cj
		uint32_t rd = instr.whole & 0x1F;        // Destination general register
		if (rd == 0) return; // Writes to x0 are discarded
		uint32_t cj = (instr.whole >> 5) & 0x7;  // Source FCC register (3 bits)
		cpu.reg(rd) = cpu.registers().cf(cj);  // Write condition flag, zero-extended
	}

	static void FCMP_COND_S(cpu_t& cpu, la_instruction instr) {
		// Floating-point compare with condition (single precision)
		// Format: fcmp.cond.s cc, fj, fk
		uint32_t cd = instr.whole & 0x7;         // FCC register index (3 bits)
		uint32_t fj = (instr.whole >> 5) & 0x1F; // Source register 1
		uint32_t fk = (instr.whole >> 10) & 0x1F; // Source register 2
		uint32_t cond = (instr.whole >> 15) & 0x1F; // Condition code (5 bits)

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		float fj_val = vr_j.f[0];
		float fk_val = vr_k.f[0];

		bool is_unordered = std::isnan(fj_val) || std::isnan(fk_val);
		bool result = false;

		switch (cond) {
			case 0x02: // CLT - (Quiet) Less Than (ordered)
			case 0x03: // SLT - Signaling Less Than (ordered)
				result = !is_unordered && (fj_val < fk_val);
				break;
			case 0x04: // CEQ - Equal (ordered)
			case 0x05: // SEQ - Signaling Equal (ordered)
				result = !is_unordered && (fj_val == fk_val);
				break;
			case 0x06: // CLE - (Quiet) Less or Equal (ordered)
			case 0x07: // SLE - Signaling Less or Equal (ordered)
				result = !is_unordered && (fj_val <= fk_val);
				break;
			case 0x0E: // CULE - (Quiet) Unordered or Less or Equal
			case 0x0F: // SULE - Signaling Unordered or Less or Equal
				result = is_unordered || (fj_val <= fk_val);
				break;
			case 0x14: // COR - (Quiet) Ordered
				result = !is_unordered;
				break;
			case 0x18: // CUNE - (Quiet) Unordered or Not Equal
			case 0x19: // SUNE - Signaling Unordered or Not Equal
				result = is_unordered || (fj_val != fk_val);
				break;
			default:
				// Unknown condition code - this should not happen in normal execution
				result = false;
				break;
		}

		cpu.registers().set_cf(cd, result ? 1 : 0);
	}

	static void FCMP_COND_D(cpu_t& cpu, la_instruction instr) {
		// Floating-point compare with condition (double precision)
		// Format: fcmp.cond.d cc, fj, fk
		uint32_t cd = instr.whole & 0x7;         // FCC register index (3 bits)
		uint32_t fj = (instr.whole >> 5) & 0x1F; // Source register 1
		uint32_t fk = (instr.whole >> 10) & 0x1F; // Source register 2
		uint32_t cond = (instr.whole >> 15) & 0x1F; // Condition code (5 bits)

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		double fj_val = vr_j.df[0];
		double fk_val = vr_k.df[0];

		bool is_unordered = std::isnan(fj_val) || std::isnan(fk_val);
		bool result = false;

		switch (cond) {
			case 0x02: // CLT - (Quiet) Less Than (ordered)
			case 0x03: // SLT - Signaling Less Than (ordered)
				result = !is_unordered && (fj_val < fk_val);
				break;
			case 0x04: // CEQ - Equal (ordered)
			case 0x05: // SEQ - Signaling Equal (ordered)
				result = !is_unordered && (fj_val == fk_val);
				break;
			case 0x06: // CLE - (Quiet) Less or Equal (ordered)
			case 0x07: // SLE - Signaling Less or Equal (ordered)
				result = !is_unordered && (fj_val <= fk_val);
				break;
			case 0x0E: // CULE - (Quiet) Unordered or Less or Equal
			case 0x0F: // SULE - Signaling Unordered or Less or Equal
				result = is_unordered || (fj_val <= fk_val);
				break;
			case 0x14: // COR - (Quiet) Ordered
				result = !is_unordered;
				break;
			case 0x18: // CUNE - (Quiet) Unordered or Not Equal
			case 0x19: // SUNE - Signaling Unordered or Not Equal
				result = is_unordered || (fj_val != fk_val);
				break;
			default:
				// Unknown condition code - this should not happen in normal execution
				result = false;
				break;
		}

		cpu.registers().set_cf(cd, result ? 1 : 0);
	}

	static void VFCMP_COND_D(cpu_t& cpu, la_instruction instr) {
		// Vector floating-point compare (double)
		// Compares each double-precision element and sets result mask
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		uint32_t cond = (instr.whole >> 15) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		// For each double element: if src1[i] < src2[i] is TRUE, set to all 1s; if FALSE, set to 0
		for (int i = 0; i < 2; i++) {
			double val1 = src1.df[i];
			double val2 = src2.df[i];
			switch (cond) {
				case 0x02: // CLT - (Quiet) Less Than (ordered)
				case 0x03: // SLT - Signaling Less Than (ordered)
					if (std::isnan(val1) || std::isnan(val2)) {
						dst.du[i] = 0;
					} else {
						dst.du[i] = (val1 < val2) ? 0xFFFFFFFFFFFFFFFFULL : 0;
					}
					break;
				case 0x04: // CEQ - Equal (ordered)
				case 0x05: // SEQ - Signaling Equal (ordered)
					if (std::isnan(val1) || std::isnan(val2)) {
						dst.du[i] = 0;
					} else {
						dst.du[i] = (val1 == val2) ? 0xFFFFFFFFFFFFFFFFULL : 0;
					}
					break;
				case 0x06: // CLE - (Quiet) Less or Equal (ordered)
				case 0x07: // SLE - Signaling Less or Equal (ordered)
					if (std::isnan(val1) || std::isnan(val2)) {
						dst.du[i] = 0;
					} else {
						dst.du[i] = (val1 <= val2) ? 0xFFFFFFFFFFFFFFFF : 0;
					}
					break;
				case 0x0E: // CULE - (Quiet) Unordered or Less or Equal
				case 0x0F: // SULE - Signaling Unordered or Less or Equal
					if (std::isnan(val1) || std::isnan(val2)) {
						dst.du[i] = 0xFFFFFFFFFFFFFFFFULL;
					} else {
						dst.du[i] = (val1 <= val2) ? 0xFFFFFFFFFFFFFFFF : 0;
					}
					break;
				case 0x14: // COR - (Quiet) Ordered
					if (std::isnan(val1) || std::isnan(val2)) {
						dst.du[i] = 0;
					} else {
						dst.du[i] = 0xFFFFFFFFFFFFFFFFULL;
					}
					break;
				case 0x18: // CUNE - (Quiet) Unordered or Not Equal
				case 0x19: // SUNE - Signaling Unordered or Not Equal
					if (std::isnan(val1) || std::isnan(val2)) {
						dst.du[i] = 0xFFFFFFFFFFFFFFFFULL;
					} else {
						dst.du[i] = (val1 != val2) ? 0xFFFFFFFFFFFFFFFF : 0;
					}
					break;
				default:
					// For simplicity, only implement less-than condition here
					dst.du[i] = 0;
					break;
			}
		}
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void FSEL(cpu_t& cpu, la_instruction instr) {
		// Floating-point conditional select: fd = (FCC[ca] != 0) ? fk : fj
		// LoongArch semantics: when condition is true, select fk; when false, select fj
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		uint32_t fk = (instr.whole >> 10) & 0x1F;
		uint32_t ca = (instr.whole >> 15) & 0x7;  // Condition flag index (3 bits)

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		auto& vr_d = cpu.registers().getvr(fd);

		// Select based on condition flag
		bool cond = (cpu.registers().cf(ca) != 0);
		vr_d.df[0] = cond ? vr_k.df[0] : vr_j.df[0];
	}

	static void FABS_D(cpu_t& cpu, la_instruction instr) {
		// Floating-point absolute value (double precision)
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;

		const auto& vr_j = cpu.registers().getvr(fj);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.df[0] = std::fabs(vr_j.df[0]);
	}

	static void FNEG_D(cpu_t& cpu, la_instruction instr) {
		// Floating-point negate (double precision)
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;

		const auto& vr_j = cpu.registers().getvr(fj);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.df[0] = -vr_j.df[0];
	}

	static void FMOV_D(cpu_t& cpu, la_instruction instr) {
		// Floating-point move (double precision)
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;

		const auto& vr_j = cpu.registers().getvr(fj);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.df[0] = vr_j.df[0];
	}

	static void FCLASS_S(cpu_t& cpu, la_instruction instr) {
		// Classify single-precision floating-point value
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;

		const auto& vr_j = cpu.registers().getvr(fj);
		auto& vr_d = cpu.registers().getvr(fd);

		float val = vr_j.f[0];
		int fpclass = std::fpclassify(val);
		bool is_neg = std::signbit(val);

		// FCLASS returns a 10-bit mask indicating the class
		uint32_t result = 0;
		if (fpclass == FP_NAN) {
			if (std::isnan(val)) result = is_neg ? 0x001 : 0x200; // sNaN or qNaN
		} else if (fpclass == FP_INFINITE) {
			result = is_neg ? 0x002 : 0x100;
		} else if (fpclass == FP_ZERO) {
			result = is_neg ? 0x004 : 0x080;
		} else if (fpclass == FP_SUBNORMAL) {
			result = is_neg ? 0x008 : 0x040;
		} else if (fpclass == FP_NORMAL) {
			result = is_neg ? 0x010 : 0x020;
		}
		vr_d.wu[0] = result;
	}

	static void FCLASS_D(cpu_t& cpu, la_instruction instr) {
		// Classify double-precision floating-point value
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;

		const auto& vr_j = cpu.registers().getvr(fj);
		auto& vr_d = cpu.registers().getvr(fd);

		double val = vr_j.df[0];
		int fpclass = std::fpclassify(val);
		bool is_neg = std::signbit(val);

		// FCLASS returns a 10-bit mask indicating the class
		uint64_t result = 0;
		if (fpclass == FP_NAN) {
			if (std::isnan(val)) result = is_neg ? 0x001 : 0x200; // sNaN or qNaN
		} else if (fpclass == FP_INFINITE) {
			result = is_neg ? 0x002 : 0x100;
		} else if (fpclass == FP_ZERO) {
			result = is_neg ? 0x004 : 0x080;
		} else if (fpclass == FP_SUBNORMAL) {
			result = is_neg ? 0x008 : 0x040;
		} else if (fpclass == FP_NORMAL) {
			result = is_neg ? 0x010 : 0x020;
		}
		vr_d.du[0] = result;
	}

	static void FFINT_D_L(cpu_t& cpu, la_instruction instr) {
		// Convert 64-bit signed integer to double-precision float
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;

		const auto& vr_j = cpu.registers().getvr(fj);
		auto& vr_d = cpu.registers().getvr(fd);
		// Convert signed 64-bit integer to double
		int64_t int_val = static_cast<int64_t>(vr_j.du[0]);
		vr_d.df[0] = static_cast<double>(int_val);
	}

	static void FFINT_D_W(cpu_t& cpu, la_instruction instr) {
		// Convert 32-bit signed integer to double-precision float
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;

		const auto& vr_j = cpu.registers().getvr(fj);
		auto& vr_d = cpu.registers().getvr(fd);
		// Convert signed 32-bit integer to double
		int32_t int_val = static_cast<int32_t>(vr_j.wu[0]);
		vr_d.df[0] = static_cast<double>(int_val);
	}

	static void FFINT_S_W(cpu_t& cpu, la_instruction instr) {
		// Convert 32-bit signed integer to single-precision float
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;

		const auto& vr_j = cpu.registers().getvr(fj);
		auto& vr_d = cpu.registers().getvr(fd);
		// Convert signed 32-bit integer to float
		int32_t int_val = static_cast<int32_t>(vr_j.wu[0]);
		vr_d.f[0] = static_cast<float>(int_val);
	}

	static void FFINT_S_L(cpu_t& cpu, la_instruction instr) {
		// Convert 64-bit signed integer to single-precision float
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;

		const auto& vr_j = cpu.registers().getvr(fj);
		auto& vr_d = cpu.registers().getvr(fd);
		// Convert signed 64-bit integer to float
		int64_t int_val = static_cast<int64_t>(vr_j.du[0]);
		vr_d.f[0] = static_cast<float>(int_val);
	}

	static void FCVT_S_D(cpu_t& cpu, la_instruction instr) {
		// Convert double-precision to single-precision float
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;

		const auto& vr_j = cpu.registers().getvr(fj);
		auto& vr_d = cpu.registers().getvr(fd);
		// Convert double to float
		vr_d.f[0] = static_cast<float>(vr_j.df[0]);
	}

	static void FCVT_D_S(cpu_t& cpu, la_instruction instr) {
		// Convert single-precision to double-precision float
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;

		const auto& vr_j = cpu.registers().getvr(fj);
		auto& vr_d = cpu.registers().getvr(fd);
		// Convert float to double
		vr_d.df[0] = static_cast<double>(vr_j.f[0]);
	}

	static void FTINTRZ_W_S(cpu_t& cpu, la_instruction instr) {
		// Convert single to 32-bit integer with truncation (round towards zero)
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;

		const auto& vr_j = cpu.registers().getvr(fj);
		auto& vr_d = cpu.registers().getvr(fd);
		// Truncate towards zero (trunc)
		int32_t int_val = static_cast<int32_t>(std::trunc(vr_j.f[0]));
		// Store as 32-bit value in low word
		vr_d.w[0] = int_val;
	}

	static void FTINTRZ_W_D(cpu_t& cpu, la_instruction instr) {
		// Convert double to 32-bit integer with truncation (round towards zero)
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;

		const auto& vr_j = cpu.registers().getvr(fj);
		auto& vr_d = cpu.registers().getvr(fd);
		// Truncate towards zero (trunc)
		int32_t int_val = static_cast<int32_t>(std::trunc(vr_j.df[0]));
		// Store as 32-bit value in low word
		vr_d.w[0] = int_val;
	}

	static void FTINTRZ_L_S(cpu_t& cpu, la_instruction instr) {
		// Convert single to 64-bit integer with truncation (round towards zero)
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;

		const auto& vr_j = cpu.registers().getvr(fj);
		auto& vr_d = cpu.registers().getvr(fd);
		// Truncate towards zero (trunc)
		int64_t int_val = static_cast<int64_t>(std::trunc(vr_j.f[0]));
		// Store as 64-bit value
		vr_d.d[0] = int_val;
	}

	static void FTINTRZ_L_D(cpu_t& cpu, la_instruction instr) {
		// Convert double to 64-bit integer with truncation (round towards zero)
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;

		const auto& vr_j = cpu.registers().getvr(fj);
		auto& vr_d = cpu.registers().getvr(fd);
		// Truncate towards zero (trunc)
		int64_t int_val = static_cast<int64_t>(std::trunc(vr_j.df[0]));
		// Store as 64-bit value
		vr_d.d[0] = int_val;
	}

	static void FADD_D(cpu_t& cpu, la_instruction instr) {
		// Floating-point add (double precision)
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.df[0] = vr_j.df[0] + vr_k.df[0];
	}

	static void FMUL_D(cpu_t& cpu, la_instruction instr) {
		// Floating-point multiply (double precision)
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.df[0] = vr_j.df[0] * vr_k.df[0];
	}

	static void FMUL_S(cpu_t& cpu, la_instruction instr) {
		// Floating-point multiply (single precision)
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.f[0] = vr_j.f[0] * vr_k.f[0];
	}

	static void FMOV_S(cpu_t& cpu, la_instruction instr) {
		// Floating-point move (single precision)
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;

		const auto& vr_j = cpu.registers().getvr(fj);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.f[0] = vr_j.f[0];
	}

	static void FADD_S(cpu_t& cpu, la_instruction instr) {
		// Floating-point add (single precision)
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.f[0] = vr_j.f[0] + vr_k.f[0];
	}

	static void FSUB_S(cpu_t& cpu, la_instruction instr) {
		// Floating-point subtract (single precision)
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.f[0] = vr_j.f[0] - vr_k.f[0];
	}

	static void FDIV_S(cpu_t& cpu, la_instruction instr) {
		// Floating-point divide (single precision)
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.f[0] = vr_j.f[0] / vr_k.f[0];
	}

	static void FMAX_S(cpu_t& cpu, la_instruction instr) {
		// Floating-point maximum (single precision)
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.f[0] = std::fmax(vr_j.f[0], vr_k.f[0]);
	}

	static void FMIN_S(cpu_t& cpu, la_instruction instr) {
		// Floating-point minimum (single precision)
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.f[0] = std::fmin(vr_j.f[0], vr_k.f[0]);
	}

	static void FMAX_D(cpu_t& cpu, la_instruction instr) {
		// Floating-point maximum (double precision)
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.d[0] = std::fmax(vr_j.d[0], vr_k.d[0]);
	}

	static void FMIN_D(cpu_t& cpu, la_instruction instr) {
		// Floating-point minimum (double precision)
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.d[0] = std::fmin(vr_j.d[0], vr_k.d[0]);
	}

	static void FABS_S(cpu_t& cpu, la_instruction instr) {
		// Floating-point absolute value (single precision)
		uint32_t fd = instr.whole & 0x1F;
		uint32_t fj = (instr.whole >> 5) & 0x1F;

		const auto& vr_j = cpu.registers().getvr(fj);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.f[0] = std::fabs(vr_j.f[0]);
	}

	static void FMADD_S(cpu_t& cpu, la_instruction instr) {
		// Fused multiply-add (single precision): fd = fa + fj * fk
		// 4R-type: fd[4:0], fj[9:5], fk[14:10], fa[19:15]
		uint32_t fd = instr.r4.rd;
		uint32_t fj = instr.r4.rj;
		uint32_t fk = instr.r4.rk;
		uint32_t fa = instr.r4.ra;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		const auto& vr_a = cpu.registers().getvr(fa);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.f[0] = vr_a.f[0] + vr_j.f[0] * vr_k.f[0];
	}

	static void FMSUB_S(cpu_t& cpu, la_instruction instr) {
		// Fused multiply-subtract (single precision): fd = fk * fj - fa
		// 4R-type: fd[4:0], fj[9:5], fk[14:10], fa[19:15]
		uint32_t fd = instr.r4.rd;
		uint32_t fj = instr.r4.rj;
		uint32_t fk = instr.r4.rk;
		uint32_t fa = instr.r4.ra;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		const auto& vr_a = cpu.registers().getvr(fa);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.f[0] = vr_k.f[0] * vr_j.f[0] - vr_a.f[0];
	}

	static void FLDX_S(cpu_t& cpu, la_instruction instr) {
		// Floating-point indexed load (single precision)
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		auto& vr = cpu.registers().getvr(instr.r3.rd);
		vr.wu[0] = cpu.memory().template read<uint32_t, true>(addr);
		vr.wu[1] = 0;
	}

	static void FSTX_S(cpu_t& cpu, la_instruction instr) {
		// Floating-point indexed store (single precision)
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		const auto& vr = cpu.registers().getvr(instr.r3.rd);
		cpu.memory().template write<uint32_t, true>(addr, vr.wu[0]);
	}

	static void FSUB_D(cpu_t& cpu, la_instruction instr) {
		// Floating-point subtract (double precision)
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.df[0] = vr_j.df[0] - vr_k.df[0];
	}

	static void FDIV_D(cpu_t& cpu, la_instruction instr) {
		// Floating-point divide (double precision)
		uint32_t fd = instr.r3.rd;
		uint32_t fj = instr.r3.rj;
		uint32_t fk = instr.r3.rk;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.df[0] = vr_j.df[0] / vr_k.df[0];
	}

	static void FMSUB_D(cpu_t& cpu, la_instruction instr) {
		// Fused multiply-subtract (double precision): fd = fk * fj - fa
		// 4R-type: fd[4:0], fj[9:5], fk[14:10], fa[19:15]
		uint32_t fd = instr.r4.rd;
		uint32_t fj = instr.r4.rj;
		uint32_t fk = instr.r4.rk;
		uint32_t fa = instr.r4.ra;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		const auto& vr_a = cpu.registers().getvr(fa);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.df[0] = vr_k.df[0] * vr_j.df[0] - vr_a.df[0];
	}

	static void FMADD_D(cpu_t& cpu, la_instruction instr) {
		// Fused multiply-add (double precision): fd = fd + fj * fk
		// 4R-type: fd[4:0], fj[9:5], fk[14:10], fa[19:15]
		uint32_t fd = instr.r4.rd;
		uint32_t fj = instr.r4.rj;
		uint32_t fk = instr.r4.rk;
		uint32_t fa = instr.r4.ra;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		const auto& vr_a = cpu.registers().getvr(fa);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.df[0] = vr_a.df[0] + vr_j.df[0] * vr_k.df[0];
	}

	static void FNMADD_S(cpu_t& cpu, la_instruction instr) {
		// Fused negative multiply-add (single precision): fd = -(fa + fj * fk)
		// 4R-type: fd[4:0], fj[9:5], fk[14:10], fa[19:15]
		uint32_t fd = instr.r4.rd;
		uint32_t fj = instr.r4.rj;
		uint32_t fk = instr.r4.rk;
		uint32_t fa = instr.r4.ra;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		const auto& vr_a = cpu.registers().getvr(fa);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.f[0] = -(vr_a.f[0] + vr_j.f[0] * vr_k.f[0]);
	}

	static void FNMADD_D(cpu_t& cpu, la_instruction instr) {
		// Fused negative multiply-add (double precision): fd = -(fa + fj * fk)
		// 4R-type: fd[4:0], fj[9:5], fk[14:10], fa[19:15]
		uint32_t fd = instr.r4.rd;
		uint32_t fj = instr.r4.rj;
		uint32_t fk = instr.r4.rk;
		uint32_t fa = instr.r4.ra;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		const auto& vr_a = cpu.registers().getvr(fa);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.df[0] = -(vr_a.df[0] + vr_j.df[0] * vr_k.df[0]);
	}

	static void FNMSUB_S(cpu_t& cpu, la_instruction instr) {
		// Fused negative multiply-subtract (single precision): fd = -(fj * fk - fa) = fa - fj * fk
		// 4R-type: fd[4:0], fj[9:5], fk[14:10], fa[19:15]
		uint32_t fd = instr.r4.rd;
		uint32_t fj = instr.r4.rj;
		uint32_t fk = instr.r4.rk;
		uint32_t fa = instr.r4.ra;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		const auto& vr_a = cpu.registers().getvr(fa);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.f[0] = vr_a.f[0] - vr_j.f[0] * vr_k.f[0];
	}

	static void FNMSUB_D(cpu_t& cpu, la_instruction instr) {
		// Fused negative multiply-subtract (double precision): fd = -(fj * fk - fa) = fa - fj * fk
		// 4R-type: fd[4:0], fj[9:5], fk[14:10], fa[19:15]
		uint32_t fd = instr.r4.rd;
		uint32_t fj = instr.r4.rj;
		uint32_t fk = instr.r4.rk;
		uint32_t fa = instr.r4.ra;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		const auto& vr_a = cpu.registers().getvr(fa);
		auto& vr_d = cpu.registers().getvr(fd);
		vr_d.df[0] = vr_a.df[0] - vr_j.df[0] * vr_k.df[0];
	}

	static void CLO_W(cpu_t& cpu, la_instruction instr) {
		uint32_t val = static_cast<uint32_t>(cpu.reg(instr.r2.rj));
		cpu.reg(instr.r2.rd) = ~val ? __builtin_clz(~val) : 32;
	}

	static void CLO_D(cpu_t& cpu, la_instruction instr) {
		uint64_t val = cpu.reg(instr.r2.rj);
		cpu.reg(instr.r2.rd) = ~val ? __builtin_clzll(~val) : 64;
	}

	static void CLZ_W(cpu_t& cpu, la_instruction instr) {
		uint32_t val = static_cast<uint32_t>(cpu.reg(instr.r2.rj));
		cpu.reg(instr.r2.rd) = val ? __builtin_clz(val) : 32;
	}

	static void CLZ_D(cpu_t& cpu, la_instruction instr) {
		uint64_t val = cpu.reg(instr.r2.rj);
		cpu.reg(instr.r2.rd) = val ? __builtin_clzll(val) : 64;
	}

	static void CTO_W(cpu_t& cpu, la_instruction instr) {
		uint32_t val = static_cast<uint32_t>(cpu.reg(instr.r2.rj));
		cpu.reg(instr.r2.rd) = ~val ? __builtin_ctz(~val) : 32;
	}

	static void CTO_D(cpu_t& cpu, la_instruction instr) {
		uint64_t val = cpu.reg(instr.r2.rj);
		cpu.reg(instr.r2.rd) = ~val ? __builtin_ctzll(~val) : 64;
	}

	static void CTZ_W(cpu_t& cpu, la_instruction instr) {
		uint32_t val = static_cast<uint32_t>(cpu.reg(instr.r2.rj));
		cpu.reg(instr.r2.rd) = val ? __builtin_ctz(val) : 32;
	}

	static void CTZ_D(cpu_t& cpu, la_instruction instr) {
		uint64_t val = cpu.reg(instr.r2.rj);
		cpu.reg(instr.r2.rd) = val ? __builtin_ctzll(val) : 64;
	}

	static void REVB_2H(cpu_t& cpu, la_instruction instr) {
		uint32_t val = static_cast<uint32_t>(cpu.reg(instr.r2.rj));
		uint32_t result = ((val & 0x00FF00FF) << 8) | ((val & 0xFF00FF00) >> 8);
		cpu.reg(instr.r2.rd) = static_cast<int64_t>(static_cast<int32_t>(result));
	}

	static void REVB_4H(cpu_t& cpu, la_instruction instr) {
		uint64_t val = cpu.reg(instr.r2.rj);
		uint64_t result = ((val & 0x00FF00FF00FF00FFULL) << 8) | ((val & 0xFF00FF00FF00FF00ULL) >> 8);
		cpu.reg(instr.r2.rd) = result;
	}

	static void REVB_2W(cpu_t& cpu, la_instruction instr) {
		uint64_t val = cpu.reg(instr.r2.rj);
		uint64_t lo = __builtin_bswap32(val & 0xFFFFFFFF);
		uint64_t hi = __builtin_bswap32(val >> 32);
		cpu.reg(instr.r2.rd) = (hi << 32) | lo;
	}

	static void REVB_D(cpu_t& cpu, la_instruction instr) {
		cpu.reg(instr.r2.rd) = __builtin_bswap64(cpu.reg(instr.r2.rj));
	}

	static void REVH_2W(cpu_t& cpu, la_instruction instr) {
		uint64_t val = cpu.reg(instr.r2.rj);
		uint32_t lo = static_cast<uint32_t>(val);
		uint32_t hi = static_cast<uint32_t>(val >> 32);
		lo = ((lo & 0xFFFF) << 16) | ((lo >> 16) & 0xFFFF);
		hi = ((hi & 0xFFFF) << 16) | ((hi >> 16) & 0xFFFF);
		cpu.reg(instr.r2.rd) = (static_cast<uint64_t>(hi) << 32) | lo;
	}

	static void REVH_D(cpu_t& cpu, la_instruction instr) {
		uint64_t val = cpu.reg(instr.r2.rj);
		uint64_t result =
			((val & 0x000000000000FFFFULL) << 48) |
			((val & 0x00000000FFFF0000ULL) << 16) |
			((val & 0x0000FFFF00000000ULL) >> 16) |
			((val & 0xFFFF000000000000ULL) >> 48);
		cpu.reg(instr.r2.rd) = result;
	}

	static void BITREV_4B(cpu_t& cpu, la_instruction instr) {
		uint32_t val = static_cast<uint32_t>(cpu.reg(instr.r2.rj));
		uint32_t result = 0;
		for (int i = 0; i < 4; i++) {
			uint8_t byte = (val >> (i * 8)) & 0xFF;
			uint8_t rev = 0;
			for (int j = 0; j < 8; j++) {
				if (byte & (1 << j)) rev |= (1 << (7 - j));
			}
			result |= (rev << (i * 8));
		}
		cpu.reg(instr.r2.rd) = static_cast<int64_t>(static_cast<int32_t>(result));
	}

	static void BITREV_8B(cpu_t& cpu, la_instruction instr) {
		uint64_t val = cpu.reg(instr.r2.rj);
		uint64_t result = 0;
		for (int i = 0; i < 8; i++) {
			uint8_t byte = (val >> (i * 8)) & 0xFF;
			uint8_t rev = 0;
			for (int j = 0; j < 8; j++) {
				if (byte & (1 << j)) rev |= (1 << (7 - j));
			}
			result |= (static_cast<uint64_t>(rev) << (i * 8));
		}
		cpu.reg(instr.r2.rd) = result;
	}

	static void BITREV_W(cpu_t& cpu, la_instruction instr) {
		uint32_t val = static_cast<uint32_t>(cpu.reg(instr.r2.rj));
		uint32_t result = 0;
		for (int i = 0; i < 32; i++) {
			if (val & (1u << i)) result |= (1u << (31 - i));
		}
		cpu.reg(instr.r2.rd) = static_cast<int64_t>(static_cast<int32_t>(result));
	}

	static void BITREV_D(cpu_t& cpu, la_instruction instr) {
		uint64_t val = cpu.reg(instr.r2.rj);
		uint64_t result = 0;
		for (int i = 0; i < 64; i++) {
			if (val & (1ULL << i)) result |= (1ULL << (63 - i));
		}
		cpu.reg(instr.r2.rd) = result;
	}

	// === ALSL.W instruction ===

	static void ALSL_W(cpu_t& cpu, la_instruction instr) {
		// ALSL.W: GR[rd] = SignExtend((GR[rj] << (sa2 + 1)) + GR[rk], 32)
		uint32_t shift = instr.r3sa2.sa2 + 1;
		int32_t result = (static_cast<int32_t>(cpu.reg(instr.r3sa2.rj)) << shift) + static_cast<int32_t>(cpu.reg(instr.r3sa2.rk));
		cpu.reg(instr.r3sa2.rd) = static_cast<int64_t>(result);
	}

	// === Vector Load/Store Instructions (LSX 128-bit) ===

	static void VLD(cpu_t& cpu, la_instruction instr) {
		// VLD vd, rj, si12
		// Load 128-bit vector from memory
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
		auto& vr = cpu.registers().getvr(instr.ri12.rd);
		vr.du[0] = cpu.memory().template read<uint64_t, true>(addr);
		vr.du[1] = cpu.memory().template read<uint64_t, true>(addr + 8);
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		vr.du[2] = 0;
		vr.du[3] = 0;
	}

	static void VST(cpu_t& cpu, la_instruction instr) {
		// VST vd, rj, si12
		// Store 128-bit vector to memory
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
		const auto& vr = cpu.registers().getvr(instr.ri12.rd);
		cpu.memory().template write<uint64_t, true>(addr, vr.du[0]);
		cpu.memory().template write<uint64_t, true>(addr + 8, vr.du[1]);
	}

	static void XVLD(cpu_t& cpu, la_instruction instr) {
		// XVLD xd, rj, si12
		// Load 256-bit LASX vector from memory
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
		auto& vr = cpu.registers().getvr(instr.ri12.rd);
		vr.du[0] = cpu.memory().template read<uint64_t, true>(addr);
		vr.du[1] = cpu.memory().template read<uint64_t, true>(addr + 8);
		vr.du[2] = cpu.memory().template read<uint64_t, true>(addr + 16);
		vr.du[3] = cpu.memory().template read<uint64_t, true>(addr + 24);
	}

	static void XVST(cpu_t& cpu, la_instruction instr) {
		// XVST xd, rj, si12
		// Store 256-bit LASX vector to memory
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
		const auto& vr = cpu.registers().getvr(instr.ri12.rd);
		cpu.memory().template write<uint64_t, true>(addr, vr.du[0]);
		cpu.memory().template write<uint64_t, true>(addr + 8, vr.du[1]);
		cpu.memory().template write<uint64_t, true>(addr + 16, vr.du[2]);
		cpu.memory().template write<uint64_t, true>(addr + 24, vr.du[3]);
	}

	// === Additional LSX Vector Instructions ===

	static void VSETANYEQZ_B(cpu_t& cpu, la_instruction instr) {
		// VSETANYEQZ.B: Set FCC[cd] if any byte in vj equals zero
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t cd = instr.whole & 0x7;

		const auto& vr = cpu.registers().getvr(vj);
		bool any_zero = false;

		// Check all 16 bytes for zero
		for (int i = 0; i < 16; i++) {
			uint8_t byte = (vr.du[i/8] >> ((i%8) * 8)) & 0xFF;
			if (byte == 0) {
				any_zero = true;
				break;
			}
		}

		cpu.registers().set_cf(cd, any_zero ? 1 : 0);
	}

	static void VSETALLNEZ_B(cpu_t& cpu, la_instruction instr) {
		// VSETALLNEZ.B: Set FCC[cd] if all bytes in vj are non-zero
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t cd = instr.whole & 0x7;

		const auto& vr = cpu.registers().getvr(vj);
		bool all_nonzero = true;

		for (int i = 0; i < 16; i++) {
			uint8_t byte = (vr.du[i/8] >> ((i%8) * 8)) & 0xFF;
			if (byte == 0) {
				all_nonzero = false;
				break;
			}
		}

		cpu.registers().set_cf(cd, all_nonzero ? 1 : 0);
	}

	static void VMSKNZ_B(cpu_t& cpu, la_instruction instr) {
		// VMSKNZ.B: Create bitmask of non-zero bytes
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vd = instr.whole & 0x1F;

		const auto& src = cpu.registers().getvr(vj);
		auto& dst = cpu.registers().getvr(vd);

		uint16_t mask = 0;
		for (int i = 0; i < 16; i++) {
			uint8_t byte = (src.du[i/8] >> ((i%8) * 8)) & 0xFF;
			if (byte != 0) {
				mask |= (1 << i);
			}
		}

		dst.du[0] = mask;
		dst.du[1] = 0;
	}

	static void VPICKVE2GR_D(cpu_t& cpu, la_instruction instr) {
		// VPICKVE2GR.D: Pick vector element to general register (double-word)
		// Encoding: 0111 0001 0001 1010 10 ui1 vj5 rd5
		uint32_t rd = instr.whole & 0x1F;
		if (rd == 0) return; // Writes to x0 are discarded
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t ui1 = (instr.whole >> 10) & 0x1;

		const auto& src = cpu.registers().getvr(vj);
		cpu.reg(rd) = src.du[ui1];
	}

	static void VPICKVE2GR_DU(cpu_t& cpu, la_instruction instr) {
		// VPICKVE2GR.DU: Pick vector element to general register (unsigned double-word)
		// Same operation as signed for 64-bit
		uint32_t rd = instr.whole & 0x1F;
		if (rd == 0) return; // Writes to x0 are discarded
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t ui1 = (instr.whole >> 10) & 0x1;

		const auto& src = cpu.registers().getvr(vj);
		cpu.reg(rd) = src.du[ui1];
	}

	static void VPICKVE2GR_W(cpu_t& cpu, la_instruction instr) {
		// VPICKVE2GR.W: Pick vector element to general register (word)
		// Sign extends to 64 bits
		uint32_t rd = instr.whole & 0x1F;
		if (rd == 0) return; // Writes to x0 are discarded
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t ui2 = (instr.whole >> 10) & 0x3;

		const auto& src = cpu.registers().getvr(vj);
		cpu.reg(rd) = static_cast<int64_t>(static_cast<int32_t>(src.wu[ui2]));
	}

	static void VPICKVE2GR_WU(cpu_t& cpu, la_instruction instr) {
		// VPICKVE2GR.WU: Pick vector element to general register (unsigned word)
		// Zero extends to 64 bits
		uint32_t rd = instr.whole & 0x1F;
		if (rd == 0) return; // Writes to x0 are discarded
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t ui2 = (instr.whole >> 10) & 0x3;

		const auto& src = cpu.registers().getvr(vj);
		cpu.reg(rd) = src.wu[ui2];
	}

	static void VPICKVE2GR_H(cpu_t& cpu, la_instruction instr) {
		// VPICKVE2GR.H: Pick vector element to general register (halfword)
		// Sign extends to 64 bits
		uint32_t rd = instr.whole & 0x1F;
		if (rd == 0) return; // Writes to x0 are discarded
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t ui3 = (instr.whole >> 10) & 0x7;

		const auto& src = cpu.registers().getvr(vj);
		cpu.reg(rd) = static_cast<int64_t>(static_cast<int16_t>(src.hu[ui3]));
	}

	static void VPICKVE2GR_HU(cpu_t& cpu, la_instruction instr) {
		// VPICKVE2GR.HU: Pick vector element to general register (unsigned halfword)
		// Zero extends to 64 bits
		uint32_t rd = instr.whole & 0x1F;
		if (rd == 0) return; // Writes to x0 are discarded
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t ui3 = (instr.whole >> 10) & 0x7;

		const auto& src = cpu.registers().getvr(vj);
		cpu.reg(rd) = src.hu[ui3];
	}

	static void VPICKVE2GR_B(cpu_t& cpu, la_instruction instr) {
		// VPICKVE2GR.B: Pick vector element to general register (byte)
		// Sign extends to 64 bits
		uint32_t rd = instr.whole & 0x1F;
		if (rd == 0) return; // Writes to x0 are discarded
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t ui4 = (instr.whole >> 10) & 0xF;

		const auto& src = cpu.registers().getvr(vj);
		cpu.reg(rd) = static_cast<int64_t>(static_cast<int8_t>(src.bu[ui4]));
	}

	static void VPICKVE2GR_BU(cpu_t& cpu, la_instruction instr) {
		// VPICKVE2GR.BU: Pick vector element to general register (unsigned byte)
		// Zero extends to 64 bits
		uint32_t rd = instr.whole & 0x1F;
		if (rd == 0) return; // Writes to x0 are discarded
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t ui4 = (instr.whole >> 10) & 0xF;

		const auto& src = cpu.registers().getvr(vj);
		cpu.reg(rd) = src.bu[ui4];
	}

	// === LSX Vector Arithmetic Instructions ===

	static void VSUB_B(cpu_t& cpu, la_instruction instr) {
		// VSUB.B: Vector subtract bytes
		// Encoding: 0000 0001 0001 0100 1 vk5 vj5 vd5
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		for (int i = 0; i < 16; i++) {
			dst.bu[i] = src1.bu[i] - src2.bu[i];
		}
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VSUB_H(cpu_t& cpu, la_instruction instr) {
		// VSUB.H: Vector subtract halfwords
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		for (int i = 0; i < 8; i++) {
			dst.hu[i] = src1.hu[i] - src2.hu[i];
		}
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VSUB_W(cpu_t& cpu, la_instruction instr) {
		// VSUB.W: Vector subtract word
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		// Subtract each 32-bit word
		dst.wu[0] = src1.wu[0] - src2.wu[0];
		dst.wu[1] = src1.wu[1] - src2.wu[1];
		dst.wu[2] = src1.wu[2] - src2.wu[2];
		dst.wu[3] = src1.wu[3] - src2.wu[3];
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.wu[4] = 0;
		dst.wu[5] = 0;
		dst.wu[6] = 0;
		dst.wu[7] = 0;
	}

	static void VSUB_D(cpu_t& cpu, la_instruction instr) {
		// VSUB.D: Vector subtract doublewords
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		dst.du[0] = src1.du[0] - src2.du[0];
		dst.du[1] = src1.du[1] - src2.du[1];
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VMUL_B(cpu_t& cpu, la_instruction instr) {
		// VMUL.B: Vector multiply bytes
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		for (int i = 0; i < 16; i++) {
			dst.bu[i] = src1.bu[i] * src2.bu[i];
		}
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VMUL_H(cpu_t& cpu, la_instruction instr) {
		// VMUL.H: Vector multiply halfwords
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		for (int i = 0; i < 8; i++) {
			dst.hu[i] = src1.hu[i] * src2.hu[i];
		}
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VMUL_W(cpu_t& cpu, la_instruction instr) {
		// VMUL.W: Vector multiply words
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		dst.wu[0] = src1.wu[0] * src2.wu[0];
		dst.wu[1] = src1.wu[1] * src2.wu[1];
		dst.wu[2] = src1.wu[2] * src2.wu[2];
		dst.wu[3] = src1.wu[3] * src2.wu[3];
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.wu[4] = 0;
		dst.wu[5] = 0;
		dst.wu[6] = 0;
		dst.wu[7] = 0;
	}

	static void VMUL_D(cpu_t& cpu, la_instruction instr) {
		// VMUL.D: Vector multiply doublewords
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		dst.du[0] = src1.du[0] * src2.du[0];
		dst.du[1] = src1.du[1] * src2.du[1];
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VMADD_B(cpu_t& cpu, la_instruction instr) {
		// VMADD.B: Vector multiply-add bytes (vd = vd + vj * vk)
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		for (int i = 0; i < 16; i++) {
			dst.bu[i] = dst.bu[i] + (src1.bu[i] * src2.bu[i]);
		}
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VMADD_H(cpu_t& cpu, la_instruction instr) {
		// VMADD.H: Vector multiply-add halfwords (vd = vd + vj * vk)
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		for (int i = 0; i < 8; i++) {
			dst.hu[i] = dst.hu[i] + (src1.hu[i] * src2.hu[i]);
		}
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VMADD_W(cpu_t& cpu, la_instruction instr) {
		// VMADD.W: Vector multiply-add words (vd = vd + vj * vk)
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		dst.wu[0] = dst.wu[0] + (src1.wu[0] * src2.wu[0]);
		dst.wu[1] = dst.wu[1] + (src1.wu[1] * src2.wu[1]);
		dst.wu[2] = dst.wu[2] + (src1.wu[2] * src2.wu[2]);
		dst.wu[3] = dst.wu[3] + (src1.wu[3] * src2.wu[3]);
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VMADD_D(cpu_t& cpu, la_instruction instr) {
		// VMADD.D: Vector multiply-add doublewords (vd = vd + vj * vk)
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		dst.du[0] = dst.du[0] + (src1.du[0] * src2.du[0]);
		dst.du[1] = dst.du[1] + (src1.du[1] * src2.du[1]);
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VADDI_HU(cpu_t& cpu, la_instruction instr) {
		// VADDI.HU: Vector add immediate unsigned halfwords
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t uk5 = (instr.whole >> 10) & 0x1F; // Unsigned 5-bit immediate

		const auto& src = cpu.registers().getvr(vj);
		auto& dst = cpu.registers().getvr(vd);

		for (int i = 0; i < 8; i++) {
			dst.hu[i] = src.hu[i] + uk5;
		}
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VADDI_WU(cpu_t& cpu, la_instruction instr) {
		// VADDI.WU: Vector add immediate unsigned words
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t uk5 = (instr.whole >> 10) & 0x1F; // Unsigned 5-bit immediate

		const auto& src = cpu.registers().getvr(vj);
		auto& dst = cpu.registers().getvr(vd);

		dst.wu[0] = src.wu[0] + uk5;
		dst.wu[1] = src.wu[1] + uk5;
		dst.wu[2] = src.wu[2] + uk5;
		dst.wu[3] = src.wu[3] + uk5;
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VADDI_DU(cpu_t& cpu, la_instruction instr) {
		// VADDI.DU: Vector add immediate unsigned doublewords
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t uk5 = (instr.whole >> 10) & 0x1F; // Unsigned 5-bit immediate

		const auto& src = cpu.registers().getvr(vj);
		auto& dst = cpu.registers().getvr(vd);

		dst.du[0] = src.du[0] + uk5;
		dst.du[1] = src.du[1] + uk5;
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VHADDW_D_W(cpu_t& cpu, la_instruction instr) {
		// VHADDW.D.W: Vector horizontal add with widening (word to doubleword)
		// Adds adjacent pairs of 32-bit signed words and produces 64-bit results
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		// Add adjacent pairs: vj[0]+vj[1], vk[0]+vk[1]
		const int64_t res1 = (int64_t)src1.w[0] + (int64_t)src1.w[1];
		const int64_t res2 = (int64_t)src2.w[0] + (int64_t)src2.w[1];
		dst.d[0] = res1;
		dst.d[1] = res2;
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.d[2] = 0;
		dst.d[3] = 0;
	}

	static void XVHADDW_D_W(cpu_t& cpu, la_instruction instr) {
		// XVHADDW.D.W: LASX vector horizontal add with widening (word to doubleword, 256-bit)
		// Adds adjacent pairs of 32-bit signed words from xj and produces 64-bit results
		// Takes all 8 words from xj, produces 4 doublewords
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		//uint32_t xk = (instr.whole >> 10) & 0x1F;

		const auto& src_j = cpu.registers().getvr(xj);
		//const auto& src_k = cpu.registers().getvr(xk);

		// Read all inputs first to handle register aliasing
		// Takes all 8 words from xj: pairs (0,1), (2,3), (4,5), (6,7) → 4 doublewords
		// xk is unused (or maybe used for other operand in 3-register form)
		int64_t r0 = (int64_t)src_j.w[0] + (int64_t)src_j.w[1];
		int64_t r1 = (int64_t)src_j.w[2] + (int64_t)src_j.w[3];
		int64_t r2 = (int64_t)src_j.w[4] + (int64_t)src_j.w[5];
		int64_t r3 = (int64_t)src_j.w[6] + (int64_t)src_j.w[7];

		auto& dst = cpu.registers().getvr(xd);
		dst.d[0] = r0;
		dst.d[1] = r1;
		dst.d[2] = r2;
		dst.d[3] = r3;
	}

	static void XVPICKVE2GR_W(cpu_t& cpu, la_instruction instr) {
		// XVPICKVE2GR.W: Pick LASX vector element to general register (word, sign-extended)
		// Selects one of 8 words from a 256-bit vector and sign-extends to 64 bits
		uint32_t rd = instr.whole & 0x1F;
		if (rd == 0) return; // Writes to x0 are discarded
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t ui3 = (instr.whole >> 10) & 0x7; // 3-bit index for 8 words

		const auto& src = cpu.registers().getvr(xj);
		cpu.reg(rd) = static_cast<int64_t>(static_cast<int32_t>(src.wu[ui3]));
	}

	static void XVADD_D(cpu_t& cpu, la_instruction instr) {
		// XVADD.D: LASX vector add doublewords (256-bit)
		// Adds corresponding 64-bit doublewords from two 256-bit vectors
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(xj);
		const auto& src2 = cpu.registers().getvr(xk);

		// Read all inputs first to handle register aliasing
		int64_t r0 = src1.d[0] + src2.d[0];
		int64_t r1 = src1.d[1] + src2.d[1];
		int64_t r2 = src1.d[2] + src2.d[2];
		int64_t r3 = src1.d[3] + src2.d[3];

		auto& dst = cpu.registers().getvr(xd);
		dst.d[0] = r0;
		dst.d[1] = r1;
		dst.d[2] = r2;
		dst.d[3] = r3;
	}

	static void XVBITSEL_V(cpu_t& cpu, la_instruction instr) {
		// XVBITSEL.V: LASX vector bit select (256-bit, 4R-type)
		// xd = (xk & xa) | (xj & ~xa)
		// When mask bit is 1, take from xk; when 0, take from xj
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;

		const auto& src_j = cpu.registers().getvr(xj);
		const auto& src_k = cpu.registers().getvr(xk);
		const auto& src_a = cpu.registers().getvr(xa);

		// Read all inputs first to handle aliasing
		uint64_t r0 = (src_a.du[0] & src_k.du[0]) | (~src_a.du[0] & src_j.du[0]);
		uint64_t r1 = (src_a.du[1] & src_k.du[1]) | (~src_a.du[1] & src_j.du[1]);
		uint64_t r2 = (src_a.du[2] & src_k.du[2]) | (~src_a.du[2] & src_j.du[2]);
		uint64_t r3 = (src_a.du[3] & src_k.du[3]) | (~src_a.du[3] & src_j.du[3]);

		auto& dst = cpu.registers().getvr(xd);
		dst.du[0] = r0;
		dst.du[1] = r1;
		dst.du[2] = r2;
		dst.du[3] = r3;
	}

	static void XVFCMP_COND_D(cpu_t& cpu, la_instruction instr) {
		// XVFCMP.COND.D: LASX vector floating-point compare (256-bit double)
		// Compares each double-precision element and sets result mask
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t cond = (instr.whole >> 15) & 0x1F;

		const auto& src1 = cpu.registers().getvr(xj);
		const auto& src2 = cpu.registers().getvr(xk);
		auto& dst = cpu.registers().getvr(xd);

		// For each of 4 double elements
		for (int i = 0; i < 4; i++) {
			double val1 = src1.df[i];
			double val2 = src2.df[i];
			switch (cond) {
				case 0x02: // CLT - (Quiet) Less Than (ordered)
				case 0x03: // SLT - Signaling Less Than (ordered)
					if (std::isnan(val1) || std::isnan(val2)) {
						dst.du[i] = 0;
					} else {
						dst.du[i] = (val1 < val2) ? 0xFFFFFFFFFFFFFFFFULL : 0;
					}
					break;
				case 0x04: // CEQ - Equal (ordered)
				case 0x05: // SEQ - Signaling Equal (ordered)
					if (std::isnan(val1) || std::isnan(val2)) {
						dst.du[i] = 0;
					} else {
						dst.du[i] = (val1 == val2) ? 0xFFFFFFFFFFFFFFFFULL : 0;
					}
					break;
				case 0x06: // CLE - (Quiet) Less or Equal (ordered)
				case 0x07: // SLE - Signaling Less or Equal (ordered)
					if (std::isnan(val1) || std::isnan(val2)) {
						dst.du[i] = 0;
					} else {
						dst.du[i] = (val1 <= val2) ? 0xFFFFFFFFFFFFFFFF : 0;
					}
					break;
				case 0x0E: // CULE - (Quiet) Unordered or Less or Equal
				case 0x0F: // SULE - Signaling Unordered or Less or Equal
					if (std::isnan(val1) || std::isnan(val2)) {
						dst.du[i] = 0xFFFFFFFFFFFFFFFFULL;
					} else {
						dst.du[i] = (val1 <= val2) ? 0xFFFFFFFFFFFFFFFF : 0;
					}
					break;
				case 0x14: // COR - (Quiet) Ordered
					if (std::isnan(val1) || std::isnan(val2)) {
						dst.du[i] = 0;
					} else {
						dst.du[i] = 0xFFFFFFFFFFFFFFFFULL;
					}
					break;
				case 0x18: // CUNE - (Quiet) Unordered or Not Equal
				case 0x19: // SUNE - Signaling Unordered or Not Equal
					if (std::isnan(val1) || std::isnan(val2)) {
						dst.du[i] = 0xFFFFFFFFFFFFFFFFULL;
					} else {
						dst.du[i] = (val1 != val2) ? 0xFFFFFFFFFFFFFFFF : 0;
					}
					break;
				default:
					dst.du[i] = 0;
					break;
			}
		}
	}

	static void XVHADDW_Q_D(cpu_t& cpu, la_instruction instr) {
		// XVHADDW.Q.D: LASX vector horizontal add with widening (doubleword to quadword, 256-bit)
		// Adds adjacent pairs of 64-bit signed doublewords and produces 128-bit results
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		//uint32_t xk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(xj);
		//const auto& src2 = cpu.registers().getvr(xk);

		// Read all inputs first to handle register aliasing
		// Takes all 4 doublewords from xj: pairs (0,1), (2,3) → 2 quadwords
		__int128 q0 = (__int128)(int64_t)src1.d[0] + (__int128)(int64_t)src1.d[1];
		__int128 q1 = (__int128)(int64_t)src1.d[2] + (__int128)(int64_t)src1.d[3];

		// Store quadwords as pairs of doublewords
		auto& dst = cpu.registers().getvr(xd);
		dst.d[0] = (int64_t)(q0 & 0xFFFFFFFFFFFFFFFFLL);
		dst.d[1] = (int64_t)(q0 >> 64);
		dst.d[2] = (int64_t)(q1 & 0xFFFFFFFFFFFFFFFFLL);
		dst.d[3] = (int64_t)(q1 >> 64);
	}

	static void VSEQ_B(cpu_t& cpu, la_instruction instr) {
		// VSEQ.B: Vector compare equal bytes (set mask)
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		for (int i = 0; i < 16; i++) {
			dst.bu[i] = (src1.bu[i] == src2.bu[i]) ? 0xFF : 0x00;
		}
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VSLT_B(cpu_t& cpu, la_instruction instr) {
		// VSLT.B: Vector signed less-than bytes (set mask)
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		for (int i = 0; i < 16; i++) {
			// Signed comparison
			dst.bu[i] = (src1.b[i] < src2.b[i]) ? 0xFF : 0x00;
		}
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VSLT_H(cpu_t& cpu, la_instruction instr) {
		// VSLT.H: Vector signed less-than halfwords (set mask)
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		for (int i = 0; i < 8; i++) {
			// Signed comparison
			dst.hu[i] = (src1.h[i] < src2.h[i]) ? 0xFFFF : 0x0000;
		}
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VSLT_W(cpu_t& cpu, la_instruction instr) {
		// VSLT.W: Vector signed less-than words (set mask)
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		dst.wu[0] = ((int32_t)src1.w[0] < (int32_t)src2.w[0]) ? 0xFFFFFFFF : 0x00000000;
		dst.wu[1] = ((int32_t)src1.w[1] < (int32_t)src2.w[1]) ? 0xFFFFFFFF : 0x00000000;
		dst.wu[2] = ((int32_t)src1.w[2] < (int32_t)src2.w[2]) ? 0xFFFFFFFF : 0x00000000;
		dst.wu[3] = ((int32_t)src1.w[3] < (int32_t)src2.w[3]) ? 0xFFFFFFFF : 0x00000000;
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VSLT_D(cpu_t& cpu, la_instruction instr) {
		// VSLT.D: Vector signed less-than doublewords (set mask)
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		dst.du[0] = ((int64_t)src1.d[0] < (int64_t)src2.d[0]) ? 0xFFFFFFFFFFFFFFFFULL : 0x0000000000000000ULL;
		dst.du[1] = ((int64_t)src1.d[1] < (int64_t)src2.d[1]) ? 0xFFFFFFFFFFFFFFFFULL : 0x0000000000000000ULL;
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VILVL_B(cpu_t& cpu, la_instruction instr) {
		// VILVL.B: Vector Interleave Low Byte
		// Interleaves the low 64-bit bytes from two vectors
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		// Interleave: dst[0] = src2[0], dst[1] = src1[0], dst[2] = src2[1], dst[3] = src1[1], ...
		// For bytes (8-bit), we interleave the low 8 elements from each source
		uint8_t result[16];
		for (int i = 0; i < 8; i++) {
			result[i*2] = src2.bu[i];
			result[i*2 + 1] = src1.bu[i];
		}
		for (int i = 0; i < 16; i++) {
			dst.bu[i] = result[i];
		}
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VILVL_H(cpu_t& cpu, la_instruction instr) {
		// VILVL.H: Vector Interleave Low Half-word
		// Interleaves the low 64-bit half-words from two vectors
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		// Interleave: dst[0] = src2[0], dst[1] = src1[0], dst[2] = src2[1], dst[3] = src1[1], ...
		// For half-words (16-bit), we interleave the low 4 elements from each source
		uint16_t result[8];
		for (int i = 0; i < 4; i++) {
			result[i*2] = src2.hu[i];
			result[i*2 + 1] = src1.hu[i];
		}
		for (int i = 0; i < 8; i++) {
			dst.hu[i] = result[i];
		}
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VILVL_W(cpu_t& cpu, la_instruction instr) {
		// VILVL.W: Vector Interleave Low Word
		// Interleaves the low 64-bit words from two vectors
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		// Interleave: dst[0] = src2[0], dst[1] = src1[0], dst[2] = src2[1], dst[3] = src1[1]
		// For words (32-bit), we interleave the low 2 elements from each source
		uint32_t result[4];
		for (int i = 0; i < 2; i++) {
			result[i*2] = src2.wu[i];
			result[i*2 + 1] = src1.wu[i];
		}
		for (int i = 0; i < 4; i++) {
			dst.wu[i] = result[i];
		}
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VILVL_D(cpu_t& cpu, la_instruction instr) {
		// VILVL.D: Vector Interleave Low Double-word
		// Interleaves the low 64-bit double-words from two vectors
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		// Interleave: dst[0] = src2[0], dst[1] = src1[0]
		// For double-words (64-bit), we interleave the low element (1 element) from each source
		dst.du[0] = src2.du[0];
		dst.du[1] = src1.du[0];
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VILVH_D(cpu_t& cpu, la_instruction instr) {
		// VILVH.D: Vector Interleave High Double-word
		// Interleaves the high 64-bit elements from two vectors
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src_j = cpu.registers().getvr(vj);
		const auto& src_k = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		// Interleave: dst[0] = src_k[1], dst[1] = src_j[1]
		dst.du[0] = src_k.du[1];
		dst.du[1] = src_j.du[1];
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VPICKEV_W(cpu_t& cpu, la_instruction instr) {
		// VPICKEV.W: Vector Pick Even Word
		// Picks even-indexed 32-bit words from two vectors
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src_j = cpu.registers().getvr(vj);
		const auto& src_k = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		// Pick even words: dst = [vk[0], vk[2], vj[0], vj[2]]
		dst.wu[0] = src_k.wu[0];
		dst.wu[1] = src_k.wu[2];
		dst.wu[2] = src_j.wu[0];
		dst.wu[3] = src_j.wu[2];
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.wu[4] = 0;
		dst.wu[5] = 0;
		dst.wu[6] = 0;
		dst.wu[7] = 0;
	}

	static void VNOR_V(cpu_t& cpu, la_instruction instr) {
		// VNOR.V: Vector NOR
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		dst.du[0] = ~(src1.du[0] | src2.du[0]);
		dst.du[1] = ~(src1.du[1] | src2.du[1]);
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VORN_V(cpu_t& cpu, la_instruction instr) {
		// VORN.V: Vector OR NOT (dst = src1 | ~src2)
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		dst.du[0] = src1.du[0] | ~src2.du[0];
		dst.du[1] = src1.du[1] | ~src2.du[1];
			// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VAND_V(cpu_t& cpu, la_instruction instr) {
		// VAND.V: Vector AND
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		dst.du[0] = src1.du[0] & src2.du[0];
		dst.du[1] = src1.du[1] & src2.du[1];
			// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VBITREVI_D(cpu_t& cpu, la_instruction instr) {
		// VBITREVI.D: Vector Bit Reverse Immediate (double)
		// XORs (toggles) a specific bit in each 64-bit element
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t imm = (instr.whole >> 10) & 0x3F;  // 6-bit immediate for bit position (0-63)

		const auto& src = cpu.registers().getvr(vj);
		auto& dst = cpu.registers().getvr(vd);

		// Toggle the specified bit in each 64-bit element
		uint64_t mask = 1ULL << imm;
		dst.du[0] = src.du[0] ^ mask;
		dst.du[1] = src.du[1] ^ mask;
			// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VLDI(cpu_t& cpu, la_instruction instr) {
		// VLDI vd, imm13
		// LSX load immediate - loads immediate pattern into 128-bit vector
		// Format: bits[4:0] = vd, bits[17:5] = imm13
		// imm13 = [mode:3][value:10] where mode determines the pattern
		uint32_t vd = instr.whole & 0x1F;
		uint32_t imm13 = (instr.whole >> 5) & 0x1FFF;

		// Extract mode (top 3 bits) and value (bottom 10 bits)
		uint32_t mode = (imm13 >> 10) & 0x7;
		uint32_t value = imm13 & 0x3FF;

		auto& dst = cpu.registers().getvr(vd);

		// Sign-extend value from 10 bits
		int64_t sext_value = (int64_t)(int16_t)(value << 6) >> 6;

		// Apply pattern based on mode
		switch (mode) {
			case 0: // Replicate 8-bit immediate to all bytes
				for (int i = 0; i < 16; i++) dst.bu[i] = (uint8_t)sext_value;
				break;
			case 1: // Replicate 16-bit immediate to all halfwords
				for (int i = 0; i < 8; i++) dst.hu[i] = (uint16_t)sext_value;
				break;
			case 2: // Replicate 32-bit immediate to all words
				for (int i = 0; i < 4; i++) dst.wu[i] = (uint32_t)sext_value;
				break;
			case 3: // Replicate 64-bit immediate to all doublewords
				for (int i = 0; i < 2; i++) dst.du[i] = (uint64_t)sext_value;
				break;
			default: // Other modes - set to zero for now
				for (int i = 0; i < 2; i++) dst.du[i] = 0;
				break;
		}
	}

	static void VORI_B(cpu_t& cpu, la_instruction instr) {
		// VORI.B: Vector OR immediate (operate on each byte)
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t imm8 = (instr.whole >> 10) & 0xFF;

		const auto& src = cpu.registers().getvr(vj);
		auto& dst = cpu.registers().getvr(vd);

		// OR immediate with each byte
		for (int i = 0; i < 16; i++) {
			dst.bu[i] = src.bu[i] | imm8;
		}
	}

	static void VFADD_D(cpu_t& cpu, la_instruction instr) {
		// VFADD.D: Vector floating-point add (double precision, 2x64-bit)
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		dst.df[0] = src1.df[0] + src2.df[0];
		dst.df[1] = src1.df[1] + src2.df[1];
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VFDIV_D(cpu_t& cpu, la_instruction instr) {
		// VFDIV.D: Vector floating-point divide (double precision, 2x64-bit)
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		dst.df[0] = src1.df[0] / src2.df[0];
		dst.df[1] = src1.df[1] / src2.df[1];
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VFMUL_S(cpu_t& cpu, la_instruction instr) {
		// VFMUL.S: Vector floating-point multiply (single precision, 4x32-bit)
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		dst.f[0] = src1.f[0] * src2.f[0];
		dst.f[1] = src1.f[1] * src2.f[1];
		dst.f[2] = src1.f[2] * src2.f[2];
		dst.f[3] = src1.f[3] * src2.f[3];
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VFMUL_D(cpu_t& cpu, la_instruction instr) {
		// VFMUL.D: Vector floating-point multiply (double precision, 2x64-bit)
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		dst.df[0] = src1.df[0] * src2.df[0];
		dst.df[1] = src1.df[1] * src2.df[1];
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VFTINTRZ_W_S(cpu_t& cpu, la_instruction instr) {
		// VFTINTRZ.W.S: Vector float to int32 with truncation towards zero (4x single-precision)
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;

		const auto& src = cpu.registers().getvr(vj);
		auto& dst = cpu.registers().getvr(vd);

		// Convert each single-precision float to int32 with truncation towards zero
		dst.w[0] = (int32_t)src.f[0];
		dst.w[1] = (int32_t)src.f[1];
		dst.w[2] = (int32_t)src.f[2];
		dst.w[3] = (int32_t)src.f[3];
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VFTINTRZ_L_D(cpu_t& cpu, la_instruction instr) {
		// VFTINTRZ.L.D: Vector double to int64 with truncation towards zero (2x double-precision)
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;

		const auto& src = cpu.registers().getvr(vj);
		auto& dst = cpu.registers().getvr(vd);

		// Convert each double-precision float to int64 with truncation towards zero
		dst.d[0] = (int64_t)src.df[0];
		dst.d[1] = (int64_t)src.df[1];
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VFMADD_D(cpu_t& cpu, la_instruction instr) {
		// VFMADD.D: Vector fused multiply-add (double precision, 2x64-bit)
		// 4R-type format: vd = va + vj * vk
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		uint32_t va = (instr.whole >> 15) & 0x1F;

		const auto& src_j = cpu.registers().getvr(vj);
		const auto& src_k = cpu.registers().getvr(vk);
		const auto& src_a = cpu.registers().getvr(va);
		auto& dst = cpu.registers().getvr(vd);

		dst.df[0] = src_a.df[0] + src_j.df[0] * src_k.df[0];
		dst.df[1] = src_a.df[1] + src_j.df[1] * src_k.df[1];
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VFNMADD_D(cpu_t& cpu, la_instruction instr) {
		// VFNMADD.D: Vector fused negative multiply-add (double precision, 2x64-bit)
		// 4R-type format: vd = -(vj * vk) + va = va - vj * vk
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		uint32_t va = (instr.whole >> 15) & 0x1F;

		const auto& src_j = cpu.registers().getvr(vj);
		const auto& src_k = cpu.registers().getvr(vk);
		const auto& src_a = cpu.registers().getvr(va);
		auto& dst = cpu.registers().getvr(vd);

		dst.df[0] = src_a.df[0] - src_j.df[0] * src_k.df[0];
		dst.df[1] = src_a.df[1] - src_j.df[1] * src_k.df[1];
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VOR_V(cpu_t& cpu, la_instruction instr) {
		// VOR.V: Vector OR
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		dst.du[0] = src1.du[0] | src2.du[0];
		dst.du[1] = src1.du[1] | src2.du[1];
			// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VXOR_V(cpu_t& cpu, la_instruction instr) {
		// VXOR.V: Vector XOR
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		dst.du[0] = src1.du[0] ^ src2.du[0];
		dst.du[1] = src1.du[1] ^ src2.du[1];
	}

	static void VSEQI_B(cpu_t& cpu, la_instruction instr) {
		// VSEQI.B vd, vj, si5
		// Set each byte to 0xFF if equal to sign-extended immediate, else 0
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		int32_t si5 = (instr.whole >> 10) & 0x1F;
		// Sign extend from 5 bits
		if (si5 & 0x10) si5 |= 0xFFFFFFE0;

		const auto& src = cpu.registers().getvr(vj);
		auto& dst = cpu.registers().getvr(vd);

		for (int i = 0; i < 2; i++) {
			uint64_t result = 0;
			for (int j = 0; j < 8; j++) {
				int8_t byte = (src.du[i] >> (j * 8)) & 0xFF;
				uint8_t cmp = (byte == (int8_t)si5) ? 0xFF : 0x00;
				result |= (uint64_t)cmp << (j * 8);
			}
			dst.du[i] = result;
		}
	}

	static void VSEQI_H(cpu_t& cpu, la_instruction instr) {
		// VSEQI.H vd, vj, si5
		// Set each halfword to 0xFFFF if equal to sign-extended immediate, else 0
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		int32_t si5 = (instr.whole >> 10) & 0x1F;
		// Sign extend from 5 bits
		if (si5 & 0x10) si5 |= 0xFFFFFFE0;

		const auto& src = cpu.registers().getvr(vj);
		auto& dst = cpu.registers().getvr(vd);

		for (int i = 0; i < 2; i++) {
			uint64_t result = 0;
			for (int j = 0; j < 4; j++) {
				int16_t halfword = (src.du[i] >> (j * 16)) & 0xFFFF;
				uint16_t cmp = (halfword == (int16_t)si5) ? 0xFFFF : 0x0000;
				result |= (uint64_t)cmp << (j * 16);
			}
			dst.du[i] = result;
		}
	}

	static void VSEQI_W(cpu_t& cpu, la_instruction instr) {
		// VSEQI.W vd, vj, si5
		// Set each word to 0xFFFFFFFF if equal to sign-extended immediate, else 0
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		int32_t si5 = (instr.whole >> 10) & 0x1F;
		// Sign extend from 5 bits
		if (si5 & 0x10) si5 |= 0xFFFFFFE0;

		const auto& src = cpu.registers().getvr(vj);
		auto& dst = cpu.registers().getvr(vd);

		for (int i = 0; i < 2; i++) {
			uint64_t result = 0;
			for (int j = 0; j < 2; j++) {
				int32_t word = (src.du[i] >> (j * 32)) & 0xFFFFFFFF;
				uint32_t cmp = (word == si5) ? 0xFFFFFFFF : 0x00000000;
				result |= (uint64_t)cmp << (j * 32);
			}
			dst.du[i] = result;
		}
	}

	static void VSEQI_D(cpu_t& cpu, la_instruction instr) {
		// VSEQI.D vd, vj, si5
		// Set each doubleword to 0xFFFFFFFFFFFFFFFF if equal to sign-extended immediate, else 0
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		int32_t si5 = (instr.whole >> 10) & 0x1F;
		// Sign extend from 5 bits
		if (si5 & 0x10) si5 |= 0xFFFFFFE0;

		const auto& src = cpu.registers().getvr(vj);
		auto& dst = cpu.registers().getvr(vd);

		dst.du[0] = ((int64_t)src.du[0] == (int64_t)si5) ? 0xFFFFFFFFFFFFFFFFULL : 0ULL;
		dst.du[1] = ((int64_t)src.du[1] == (int64_t)si5) ? 0xFFFFFFFFFFFFFFFFULL : 0ULL;
	}

	static void VFRSTPI_B(cpu_t& cpu, la_instruction instr) {
		// VFRSTPI.B vd, vj, ui5
		// Find first set position in vector (starting from ui5)
		// Sets vd[0] to the position of first non-zero byte starting from position ui5
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t ui5 = (instr.whole >> 10) & 0x1F;

		const auto& src = cpu.registers().getvr(vj);
		auto& dst = cpu.registers().getvr(vd);

		// Find first non-zero byte starting from ui5
		uint8_t pos = 16; // Default: not found
		for (int i = ui5; i < 16; i++) {
			uint8_t byte = src.bu[i];
			if (byte != 0) {
				pos = i;
				break;
			}
		}

		// Store result in first byte of vd, clear rest
		dst.du[0] = pos;
		dst.du[1] = 0;
	}

	static void BCNEZ(cpu_t& cpu, la_instruction instr) {
		// BCNEZ: Branch if condition flag is not equal to zero
		// Format: 0x48xxxxxx
		uint32_t cj = (instr.whole >> 5) & 0x7;
		int32_t offset = ((instr.whole >> 10) & 0xFFFF) << 2;
		// Sign extend
		if (offset & 0x20000) {
			offset |= 0xFFFC0000;
		}

		if (cpu.registers().cf(cj) != 0) {
			cpu.increment_pc(offset-4);
		}
	}

	static void BCEQZ(cpu_t& cpu, la_instruction instr) {
		// BCEQZ: Branch if condition flag equals zero
		uint32_t cj = (instr.whole >> 5) & 0x7;
		int32_t offset = ((instr.whole >> 10) & 0xFFFF) << 2;
		if (offset & 0x20000) {
			offset |= 0xFFFC0000;
		}

		if (cpu.registers().cf(cj) == 0) {
			cpu.increment_pc(offset-4);
		}
	}

	// === Vector Replicate Instructions ===

	static void VREPLVEI_D(cpu_t& cpu, la_instruction instr) {
		// VREPLVEI.D: Vector Replicate Vector Element Immediate (double)
		// Replicates a specified 64-bit element to all elements in the destination vector
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t idx = (instr.whole >> 10) & 0x1;  // Element index (0 or 1 for doubles)

		const auto& src = cpu.registers().getvr(vj);
		auto& dst = cpu.registers().getvr(vd);

		// Replicate the selected element to both positions
		dst.du[0] = src.du[idx];
		dst.du[1] = src.du[idx];
			// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VREPLGR2VR_B(cpu_t& cpu, la_instruction instr) {
		// VREPLGR2VR.B vd, rj
		// Replicate byte from GPR rj to all 16 bytes of vd
		uint32_t vd = instr.whole & 0x1F;
		uint32_t rj = (instr.whole >> 5) & 0x1F;

		uint8_t value = cpu.reg(rj) & 0xFF;
		auto& dst = cpu.registers().getvr(vd);

		// Fill all 16 bytes with the same value
		uint64_t replicated = 0;
		for (int i = 0; i < 8; i++) {
			replicated |= (uint64_t)value << (i * 8);
		}
		dst.du[0] = replicated;
		dst.du[1] = replicated;
	}

	static void VREPLGR2VR_H(cpu_t& cpu, la_instruction instr) {
		// VREPLGR2VR.H vd, rj
		// Replicate halfword from GPR rj to all 8 halfwords of vd
		uint32_t vd = instr.whole & 0x1F;
		uint32_t rj = (instr.whole >> 5) & 0x1F;

		uint16_t value = cpu.reg(rj) & 0xFFFF;
		auto& dst = cpu.registers().getvr(vd);

		// Fill all 8 halfwords with the same value
		for (int i = 0; i < 8; i++) {
			dst.hu[i] = value;
		}
	}

	static void VREPLGR2VR_W(cpu_t& cpu, la_instruction instr) {
		// VREPLGR2VR.W vd, rj
		// Replicate word from GPR rj to all 4 words of vd
		uint32_t vd = instr.whole & 0x1F;
		uint32_t rj = (instr.whole >> 5) & 0x1F;

		uint32_t value = cpu.reg(rj) & 0xFFFFFFFF;
		auto& dst = cpu.registers().getvr(vd);

		// Fill all 4 words with the same value
		for (int i = 0; i < 4; i++) {
			dst.wu[i] = value;
		}
	}

	static void VREPLGR2VR_D(cpu_t& cpu, la_instruction instr) {
		// VREPLGR2VR.D vd, rj
		// Replicate doubleword from GPR rj to both 64-bit elements of vd
		uint32_t vd = instr.whole & 0x1F;
		uint32_t rj = (instr.whole >> 5) & 0x1F;

		uint64_t value = cpu.reg(rj);
		auto& dst = cpu.registers().getvr(vd);

		// Fill both doublewords with the same value
		dst.du[0] = value;
		dst.du[1] = value;
	}

	static void VINSGR2VR_B(cpu_t& cpu, la_instruction instr) {
		// VINSGR2VR.B vd, rj, idx
		// Insert byte from GPR rj to byte element idx of vd
		uint32_t vd = instr.whole & 0x1F;
		uint32_t rj = (instr.whole >> 5) & 0x1F;
		uint32_t idx = (instr.whole >> 10) & 0xF;

		uint8_t value = cpu.reg(rj) & 0xFF;
		auto& dst = cpu.registers().getvr(vd);

		dst.bu[idx] = value;
	}

	static void VINSGR2VR_H(cpu_t& cpu, la_instruction instr) {
		// VINSGR2VR.H vd, rj, idx
		// Insert halfword from GPR rj to halfword element idx of vd
		uint32_t vd = instr.whole & 0x1F;
		uint32_t rj = (instr.whole >> 5) & 0x1F;
		uint32_t idx = (instr.whole >> 10) & 0x7;

		uint16_t value = cpu.reg(rj) & 0xFFFF;
		auto& dst = cpu.registers().getvr(vd);

		dst.hu[idx] = value;
	}

	static void VINSGR2VR_W(cpu_t& cpu, la_instruction instr) {
		// VINSGR2VR.W vd, rj, idx
		// Insert word from GPR rj to word element idx of vd
		uint32_t vd = instr.whole & 0x1F;
		uint32_t rj = (instr.whole >> 5) & 0x1F;
		uint32_t idx = (instr.whole >> 10) & 0x3;

		uint32_t value = cpu.reg(rj) & 0xFFFFFFFF;
		auto& dst = cpu.registers().getvr(vd);

		dst.wu[idx] = value;
	}

	static void VINSGR2VR_D(cpu_t& cpu, la_instruction instr) {
		// VINSGR2VR.D vd, rj, idx
		// Insert doubleword from GPR rj to doubleword element idx of vd
		uint32_t vd = instr.whole & 0x1F;
		uint32_t rj = (instr.whole >> 5) & 0x1F;
		uint32_t idx = (instr.whole >> 10) & 0x1;

		uint64_t value = cpu.reg(rj);
		auto& dst = cpu.registers().getvr(vd);

		dst.du[idx] = value;
	}

	static void VADDI_BU(cpu_t& cpu, la_instruction instr) {
		// VADDI.BU vd, vj, ui5
		// Add immediate to each unsigned byte
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t imm = (instr.whole >> 10) & 0x1F;

		const auto& src = cpu.registers().getvr(vj);
		auto& dst = cpu.registers().getvr(vd);

		// Add immediate to each byte (with unsigned wraparound)
		for (int i = 0; i < 2; i++) {
			uint64_t result = 0;
			for (int j = 0; j < 8; j++) {
				uint8_t byte = (src.du[i] >> (j * 8)) & 0xFF;
				uint8_t sum = byte + imm;
				result |= (uint64_t)sum << (j * 8);
			}
			dst.du[i] = result;
		}
	}

	static void VADD_B(cpu_t& cpu, la_instruction instr) {
		// VADD.B vd, vj, vk
		// Add corresponding bytes in vj and vk
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		for (int i = 0; i < 16; i++) {
			dst.bu[i] = src1.bu[i] + src2.bu[i];
		}
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VADD_H(cpu_t& cpu, la_instruction instr) {
		// VADD.H vd, vj, vk
		// Add corresponding halfwords in vj and vk
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		for (int i = 0; i < 8; i++) {
			dst.hu[i] = src1.hu[i] + src2.hu[i];
		}
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VADD_W(cpu_t& cpu, la_instruction instr) {
		// VADD.W vd, vj, vk
		// Add corresponding words in vj and vk
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		dst.wu[0] = src1.wu[0] + src2.wu[0];
		dst.wu[1] = src1.wu[1] + src2.wu[1];
		dst.wu[2] = src1.wu[2] + src2.wu[2];
		dst.wu[3] = src1.wu[3] + src2.wu[3];
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.wu[4] = 0;
		dst.wu[5] = 0;
		dst.wu[6] = 0;
		dst.wu[7] = 0;
	}

	static void VADD_D(cpu_t& cpu, la_instruction instr) {
		// VADD.D vd, vj, vk
		// Add corresponding doublewords in vj and vk
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		dst.du[0] = src1.du[0] + src2.du[0];
		dst.du[1] = src1.du[1] + src2.du[1];
		// LSX instructions zero-extend to 256 bits (clear upper 128 bits for LASX compatibility)
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void VSHUF_B(cpu_t& cpu, la_instruction instr) {
		// VSHUF.B vd, vj, vk, va
		// Shuffle bytes: for each byte in va, use low 5 bits as index into concatenated vk:vj
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		uint32_t va = (instr.whole >> 15) & 0x1F;

		const auto& src_j = cpu.registers().getvr(vj);
		const auto& src_k = cpu.registers().getvr(vk);
		const auto& idx = cpu.registers().getvr(va);
		auto& dst = cpu.registers().getvr(vd);

		// Build combined 32-byte array: [vk[15:0], vj[15:0]]
		uint8_t combined[32];
		for (int i = 0; i < 8; i++) {
			combined[i] = (src_k.du[0] >> (i * 8)) & 0xFF;
			combined[i + 8] = (src_k.du[1] >> (i * 8)) & 0xFF;
			combined[i + 16] = (src_j.du[0] >> (i * 8)) & 0xFF;
			combined[i + 24] = (src_j.du[1] >> (i * 8)) & 0xFF;
		}

		// Shuffle based on index
		for (int i = 0; i < 2; i++) {
			uint64_t result = 0;
			for (int j = 0; j < 8; j++) {
				uint8_t index = (idx.du[i] >> (j * 8)) & 0x1F;
				uint8_t byte = combined[index];
				result |= (uint64_t)byte << (j * 8);
			}
			dst.du[i] = result;
		}
	}

	static void VBITSEL_V(cpu_t& cpu, la_instruction instr) {
		// VBITSEL.V: Vector bit select (4R-type)
		// vd = (vk & va) | (vj & ~va)
		// Inverted from typical SIMD: when mask bit is 1, take from vk; when 0, take from vj
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		uint32_t va = (instr.whole >> 15) & 0x1F;

		const auto& src_j = cpu.registers().getvr(vj);
		const auto& src_k = cpu.registers().getvr(vk);
		const auto& src_a = cpu.registers().getvr(va);
		auto& dst = cpu.registers().getvr(vd);

		// Bit select: for each bit, if mask bit is 1, take from vk, else from vj
		for (int i = 0; i < 2; i++) {
			dst.du[i] = (src_k.du[i] & src_a.du[i]) | (src_j.du[i] & ~src_a.du[i]);
		}
		// LSX instructions zero-extend to 256 bits
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	// === VMAX/VMIN instructions ===

	static void VMAX_B(cpu_t& cpu, la_instruction instr) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);
		for (int i = 0; i < 16; i++)
			dst.b[i] = (src1.b[i] > src2.b[i]) ? src1.b[i] : src2.b[i];
	}

	static void VMAX_H(cpu_t& cpu, la_instruction instr) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);
		for (int i = 0; i < 8; i++)
			dst.h[i] = (src1.h[i] > src2.h[i]) ? src1.h[i] : src2.h[i];
	}

	static void VMAX_W(cpu_t& cpu, la_instruction instr) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);
		for (int i = 0; i < 4; i++)
			dst.w[i] = (src1.w[i] > src2.w[i]) ? src1.w[i] : src2.w[i];
	}

	static void VMAX_D(cpu_t& cpu, la_instruction instr) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);
		for (int i = 0; i < 2; i++)
			dst.d[i] = (src1.d[i] > src2.d[i]) ? src1.d[i] : src2.d[i];
	}

	static void VMAX_BU(cpu_t& cpu, la_instruction instr) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);
		for (int i = 0; i < 16; i++)
			dst.bu[i] = (src1.bu[i] > src2.bu[i]) ? src1.bu[i] : src2.bu[i];
	}

	static void VMAX_HU(cpu_t& cpu, la_instruction instr) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);
		for (int i = 0; i < 8; i++)
			dst.hu[i] = (src1.hu[i] > src2.hu[i]) ? src1.hu[i] : src2.hu[i];
	}

	static void VMAX_WU(cpu_t& cpu, la_instruction instr) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);
		for (int i = 0; i < 4; i++)
			dst.wu[i] = (src1.wu[i] > src2.wu[i]) ? src1.wu[i] : src2.wu[i];
	}

	static void VMAX_DU(cpu_t& cpu, la_instruction instr) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);
		for (int i = 0; i < 2; i++)
			dst.du[i] = (src1.du[i] > src2.du[i]) ? src1.du[i] : src2.du[i];
	}

	static void VMIN_B(cpu_t& cpu, la_instruction instr) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);
		for (int i = 0; i < 16; i++)
			dst.b[i] = (src1.b[i] < src2.b[i]) ? src1.b[i] : src2.b[i];
	}

	static void VMIN_H(cpu_t& cpu, la_instruction instr) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);
		for (int i = 0; i < 8; i++)
			dst.h[i] = (src1.h[i] < src2.h[i]) ? src1.h[i] : src2.h[i];
	}

	static void VMIN_W(cpu_t& cpu, la_instruction instr) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);
		for (int i = 0; i < 4; i++)
			dst.w[i] = (src1.w[i] < src2.w[i]) ? src1.w[i] : src2.w[i];
	}

	static void VMIN_D(cpu_t& cpu, la_instruction instr) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);
		for (int i = 0; i < 2; i++)
			dst.d[i] = (src1.d[i] < src2.d[i]) ? src1.d[i] : src2.d[i];
	}

	static void VMIN_BU(cpu_t& cpu, la_instruction instr) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);
		for (int i = 0; i < 16; i++)
			dst.bu[i] = (src1.bu[i] < src2.bu[i]) ? src1.bu[i] : src2.bu[i];
	}

	static void VMIN_HU(cpu_t& cpu, la_instruction instr) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);
		for (int i = 0; i < 8; i++)
			dst.hu[i] = (src1.hu[i] < src2.hu[i]) ? src1.hu[i] : src2.hu[i];
	}

	static void VMIN_WU(cpu_t& cpu, la_instruction instr) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);
		for (int i = 0; i < 4; i++)
			dst.wu[i] = (src1.wu[i] < src2.wu[i]) ? src1.wu[i] : src2.wu[i];
	}

	static void VMIN_DU(cpu_t& cpu, la_instruction instr) {
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;
		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);
		for (int i = 0; i < 2; i++)
			dst.du[i] = (src1.du[i] < src2.du[i]) ? src1.du[i] : src2.du[i];
	}


	// === LASX (256-bit) Instructions ===

	static void XVREPLGR2VR_B(cpu_t& cpu, la_instruction instr) {
		// XVREPLGR2VR.B xd, rj
		// Replicate byte from GPR rj to all 32 bytes of xd
		uint32_t xd = instr.whole & 0x1F;
		uint32_t rj = (instr.whole >> 5) & 0x1F;

		uint8_t value = cpu.reg(rj) & 0xFF;
		auto& dst = cpu.registers().getvr(xd);

		// Fill all 32 bytes with the same value
		uint64_t replicated = 0;
		for (int i = 0; i < 8; i++) {
			replicated |= (uint64_t)value << (i * 8);
		}
		dst.du[0] = replicated;
		dst.du[1] = replicated;
		dst.du[2] = replicated;
		dst.du[3] = replicated;
	}

	static void XVXOR_V(cpu_t& cpu, la_instruction instr) {
		// XVXOR.V xd, xj, xk
		// Bitwise XOR of 256-bit vectors
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(xj);
		const auto& src2 = cpu.registers().getvr(xk);
		auto& dst = cpu.registers().getvr(xd);

		dst.du[0] = src1.du[0] ^ src2.du[0];
		dst.du[1] = src1.du[1] ^ src2.du[1];
		dst.du[2] = src1.du[2] ^ src2.du[2];
		dst.du[3] = src1.du[3] ^ src2.du[3];
	}

	static void XVSUB_W(cpu_t& cpu, la_instruction instr) {
		// XVSUB.W: LASX vector subtract word (256-bit, 8x32-bit)
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(xj);
		const auto& src2 = cpu.registers().getvr(xk);
		auto& dst = cpu.registers().getvr(xd);

		// Subtract each 32-bit word (8 words in 256-bit vector)
		dst.w[0] = src1.w[0] - src2.w[0];
		dst.w[1] = src1.w[1] - src2.w[1];
		dst.w[2] = src1.w[2] - src2.w[2];
		dst.w[3] = src1.w[3] - src2.w[3];
		dst.w[4] = src1.w[4] - src2.w[4];
		dst.w[5] = src1.w[5] - src2.w[5];
		dst.w[6] = src1.w[6] - src2.w[6];
		dst.w[7] = src1.w[7] - src2.w[7];
	}

	static void XVMIN_BU(cpu_t& cpu, la_instruction instr) {
		// XVMIN.BU xd, xj, xk
		// Unsigned minimum of corresponding bytes (256-bit)
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(xj);
		const auto& src2 = cpu.registers().getvr(xk);
		auto& dst = cpu.registers().getvr(xd);

		for (int i = 0; i < 32; i++) {
			const uint8_t b1 = src1.bu[i];
			const uint8_t b2 = src2.bu[i];
			dst.bu[i] = (b1 < b2) ? b1 : b2;
		}
	}

	static void XVMAX_BU(cpu_t& cpu, la_instruction instr) {
		// XVMAX.BU xd, xj, xk
		// Unsigned maximum of corresponding bytes (256-bit)
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(xj);
		const auto& src2 = cpu.registers().getvr(xk);
		auto& dst = cpu.registers().getvr(xd);

		for (int i = 0; i < 32; i++) {
			const uint8_t b1 = src1.bu[i];
			const uint8_t b2 = src2.bu[i];
			dst.bu[i] = (b1 > b2) ? b1 : b2;
		}
	}

	static void XVMSKNZ_B(cpu_t& cpu, la_instruction instr) {
		// XVMSKNZ.B xd, xj
		// Create a 32-bit mask where each bit indicates if corresponding byte is non-zero
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;

		const auto& src = cpu.registers().getvr(xj);
		auto& dst = cpu.registers().getvr(xd);

		uint32_t mask = 0;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 8; j++) {
				uint8_t byte = (src.du[i] >> (j * 8)) & 0xFF;
				if (byte != 0) {
					mask |= (1u << (i * 8 + j));
				}
			}
		}

		dst.du[0] = mask;
		dst.du[1] = 0;
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void XVPICKVE_W(cpu_t& cpu, la_instruction instr) {
		// XVPICKVE.W xd, xj, ui3
		// Extract a specific 32-bit word from xj and place in lower word of xd
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t idx = (instr.whole >> 10) & 0x7;  // 3-bit index (0-7 for 8 words)

		const auto& src = cpu.registers().getvr(xj);
		auto& dst = cpu.registers().getvr(xd);

		// Extract the specified word
		uint32_t word;
		if (idx < 2) {
			word = (src.du[0] >> (idx * 32)) & 0xFFFFFFFF;
		} else if (idx < 4) {
			word = (src.du[1] >> ((idx - 2) * 32)) & 0xFFFFFFFF;
		} else if (idx < 6) {
			word = (src.du[2] >> ((idx - 4) * 32)) & 0xFFFFFFFF;
		} else {
			word = (src.du[3] >> ((idx - 6) * 32)) & 0xFFFFFFFF;
		}

		dst.du[0] = word;
		dst.du[1] = 0;
		dst.du[2] = 0;
		dst.du[3] = 0;
	}

	static void XVSETANYEQZ_B(cpu_t& cpu, la_instruction instr) {
		// XVSETANYEQZ.B cd, xj
		// Set FCC[cd] if any byte in xj equals zero
		uint32_t cd = instr.whole & 0x7;
		uint32_t xj = (instr.whole >> 5) & 0x1F;

		const auto& src = cpu.registers().getvr(xj);
		bool any_zero = false;

		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 8; j++) {
				uint8_t byte = (src.du[i] >> (j * 8)) & 0xFF;
				if (byte == 0) {
					any_zero = true;
					break;
				}
			}
			if (any_zero) break;
		}

		cpu.registers().set_cf(cd, any_zero ? 1 : 0);
	}

	static void XVSEQ_B(cpu_t& cpu, la_instruction instr) {
		// XVSEQ.B xd, xj, xk
		// Set each byte to 0xFF if equal, 0x00 if not
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(xj);
		const auto& src2 = cpu.registers().getvr(xk);
		auto& dst = cpu.registers().getvr(xd);

		for (int i = 0; i < 4; i++) {
			uint64_t result = 0;
			for (int j = 0; j < 8; j++) {
				uint8_t b1 = (src1.du[i] >> (j * 8)) & 0xFF;
				uint8_t b2 = (src2.du[i] >> (j * 8)) & 0xFF;
				uint8_t cmp = (b1 == b2) ? 0xFF : 0x00;
				result |= (uint64_t)cmp << (j * 8);
			}
			dst.du[i] = result;
		}
	}

	static void XVSETEQZ_V(cpu_t& cpu, la_instruction instr) {
		// XVSETEQZ.V cd, xj
		// Set FCC[cd] if entire 256-bit vector is zero
		uint32_t cd = instr.whole & 0x7;
		uint32_t xj = (instr.whole >> 5) & 0x1F;

		const auto& src = cpu.registers().getvr(xj);
		bool all_zero = (src.du[0] == 0 && src.du[1] == 0 && src.du[2] == 0 && src.du[3] == 0);

		cpu.registers().set_cf(cd, all_zero ? 1 : 0);
	}

	static void XVPERMI_Q(cpu_t& cpu, la_instruction instr) {
		// XVPERMI.Q xd, xj, ui8
		// Permute quadwords (128-bit chunks) based on immediate
		// Format: XdXjUk8 where a=xd (dest is also source), b=xj
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t imm = (instr.whole >> 10) & 0xFF;

		const auto& a = cpu.registers().getvr(xd);  // First source (also destination)
		const auto& b = cpu.registers().getvr(xj);  // Second source
		auto& dst = cpu.registers().getvr(xd);

		// Pseudo-code:
		// dst.qword[0] = (imm & 2) ? a.qword[imm & 0x1] : b.qword[imm & 0x1];
		// dst.qword[1] = (imm & 0x20) ? a.qword[(imm >> 4) & 0x1] : b.qword[(imm >> 4) & 0x1];

		// Save values before modifying dst (since dst aliases with a)
		uint64_t tmp_a[4] = {a.du[0], a.du[1], a.du[2], a.du[3]};
		uint64_t tmp_b[4] = {b.du[0], b.du[1], b.du[2], b.du[3]};

		// Select lower qword (128 bits = 2 x 64-bit elements)
		uint32_t lo_idx = imm & 0x1;
		const auto& lo_src = (imm & 2) ? tmp_a : tmp_b;
		dst.du[0] = lo_src[lo_idx * 2];
		dst.du[1] = lo_src[lo_idx * 2 + 1];

		// Select upper qword (128 bits = 2 x 64-bit elements)
		uint32_t hi_idx = (imm >> 4) & 0x1;
		const auto& hi_src = (imm & 0x20) ? tmp_a : tmp_b;
		dst.du[2] = hi_src[hi_idx * 2];
		dst.du[3] = hi_src[hi_idx * 2 + 1];
	}

	static void XVLDX(cpu_t& cpu, la_instruction instr) {
		// XVLDX xd, rj, rk
		// Vector indexed load (LASX 256-bit)
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		auto& vr = cpu.registers().getvr(instr.r3.rd);
		vr.du[0] = cpu.memory().template read<uint64_t, true>(addr);
		vr.du[1] = cpu.memory().template read<uint64_t, true>(addr + 8);
		vr.du[2] = cpu.memory().template read<uint64_t, true>(addr + 16);
		vr.du[3] = cpu.memory().template read<uint64_t, true>(addr + 24);
	}

	static void XVSTX(cpu_t& cpu, la_instruction instr) {
		// XVSTX xd, rj, rk
		// Vector indexed store (LASX 256-bit)
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		const auto& vr = cpu.registers().getvr(instr.r3.rd);
		cpu.memory().template write<uint64_t, true>(addr, vr.du[0]);
		cpu.memory().template write<uint64_t, true>(addr + 8, vr.du[1]);
		cpu.memory().template write<uint64_t, true>(addr + 16, vr.du[2]);
		cpu.memory().template write<uint64_t, true>(addr + 24, vr.du[3]);
	}

	static void XVFADD_D(cpu_t& cpu, la_instruction instr) {
		// XVFADD.D: LASX vector floating-point add (double precision, 4x64-bit)
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(xj);
		const auto& src2 = cpu.registers().getvr(xk);
		auto& dst = cpu.registers().getvr(xd);

		dst.df[0] = src1.df[0] + src2.df[0];
		dst.df[1] = src1.df[1] + src2.df[1];
		dst.df[2] = src1.df[2] + src2.df[2];
		dst.df[3] = src1.df[3] + src2.df[3];
	}

	static void XVFMUL_D(cpu_t& cpu, la_instruction instr) {
		// XVFMUL.D: LASX vector floating-point multiply (double precision, 4x64-bit)
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(xj);
		const auto& src2 = cpu.registers().getvr(xk);
		auto& dst = cpu.registers().getvr(xd);

		dst.df[0] = src1.df[0] * src2.df[0];
		dst.df[1] = src1.df[1] * src2.df[1];
		dst.df[2] = src1.df[2] * src2.df[2];
		dst.df[3] = src1.df[3] * src2.df[3];
	}

	static void XVFDIV_D(cpu_t& cpu, la_instruction instr) {
		// XVFDIV.D: LASX vector floating-point divide (double precision, 4x64-bit)
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(xj);
		const auto& src2 = cpu.registers().getvr(xk);
		auto& dst = cpu.registers().getvr(xd);

		dst.df[0] = src1.df[0] / src2.df[0];
		dst.df[1] = src1.df[1] / src2.df[1];
		dst.df[2] = src1.df[2] / src2.df[2];
		dst.df[3] = src1.df[3] / src2.df[3];
	}

	static void XVFSUB_D(cpu_t& cpu, la_instruction instr) {
		// XVFSUB.D: LASX vector floating-point subtract (double precision, 4x64-bit)
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(xj);
		const auto& src2 = cpu.registers().getvr(xk);
		auto& dst = cpu.registers().getvr(xd);

		dst.df[0] = src1.df[0] - src2.df[0];
		dst.df[1] = src1.df[1] - src2.df[1];
		dst.df[2] = src1.df[2] - src2.df[2];
		dst.df[3] = src1.df[3] - src2.df[3];
	}

	static void XVBITREVI_D(cpu_t& cpu, la_instruction instr) {
		// XVBITREVI.D: LASX vector bit reverse immediate (double precision, 4x64-bit)
		// XORs (toggles) a specific bit in each 64-bit element
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t imm = (instr.whole >> 10) & 0x3F;  // 6-bit immediate for bit position (0-63)

		const auto& src = cpu.registers().getvr(xj);
		auto& dst = cpu.registers().getvr(xd);

		// Toggle the specified bit in each 64-bit element (all 4 elements for LASX)
		uint64_t mask = 1ULL << imm;
		dst.du[0] = src.du[0] ^ mask;
		dst.du[1] = src.du[1] ^ mask;
		dst.du[2] = src.du[2] ^ mask;
		dst.du[3] = src.du[3] ^ mask;
	}

	static void XVREPLVE_D(cpu_t& cpu, la_instruction instr) {
		// XVREPLVE.D: LASX vector replicate element from register (double precision)
		// Replicates element selected by rj to all elements in xd from vector xk
		// Format: xd[i] = xk[rj % 4] for all i
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;

		const auto& src = cpu.registers().getvr(xk);
		auto& dst = cpu.registers().getvr(xd);

		// Get the element index from register rj (modulo 4 for double elements)
		uint32_t idx = cpu.reg(xj) & 0x3;

		// Replicate the selected element to all 4 positions
		uint64_t value = src.du[idx];
		dst.du[0] = value;
		dst.du[1] = value;
		dst.du[2] = value;
		dst.du[3] = value;
	}

	static void XVFMADD_S(cpu_t& cpu, la_instruction instr) {
		// XVFMADD.S: LASX vector fused multiply-add (single precision, 8x32-bit)
		// 4R-type format: xd = xa + xj * xk
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;

		const auto& src_j = cpu.registers().getvr(xj);
		const auto& src_k = cpu.registers().getvr(xk);
		const auto& src_a = cpu.registers().getvr(xa);
		auto& dst = cpu.registers().getvr(xd);

		for (int i = 0; i < 8; i++) {
			dst.f[i] = src_a.f[i] + src_j.f[i] * src_k.f[i];
		}
	}

	static void XVFMADD_D(cpu_t& cpu, la_instruction instr) {
		// XVFMADD.D: LASX vector fused multiply-add (double precision, 4x64-bit)
		// 4R-type format: xd = xa + xj * xk
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;

		const auto& src_j = cpu.registers().getvr(xj);
		const auto& src_k = cpu.registers().getvr(xk);
		const auto& src_a = cpu.registers().getvr(xa);
		auto& dst = cpu.registers().getvr(xd);

		dst.df[0] = src_a.df[0] + src_j.df[0] * src_k.df[0];
		dst.df[1] = src_a.df[1] + src_j.df[1] * src_k.df[1];
		dst.df[2] = src_a.df[2] + src_j.df[2] * src_k.df[2];
		dst.df[3] = src_a.df[3] + src_j.df[3] * src_k.df[3];
	}

	static void XVFMSUB_S(cpu_t& cpu, la_instruction instr) {
		// XVFMSUB.S: LASX vector fused multiply-subtract (single precision, 8x32-bit)
		// 4R-type format: xd = xa - xj * xk
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;

		const auto& src_j = cpu.registers().getvr(xj);
		const auto& src_k = cpu.registers().getvr(xk);
		const auto& src_a = cpu.registers().getvr(xa);
		auto& dst = cpu.registers().getvr(xd);

		for (int i = 0; i < 8; i++) {
			dst.f[i] = src_a.f[i] - src_j.f[i] * src_k.f[i];
		}
	}

	static void XVFMSUB_D(cpu_t& cpu, la_instruction instr) {
		// XVFMSUB.D: LASX vector fused multiply-subtract (double precision, 4x64-bit)
		// 4R-type format: xd = xa - xj * xk
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;

		const auto& src_j = cpu.registers().getvr(xj);
		const auto& src_k = cpu.registers().getvr(xk);
		const auto& src_a = cpu.registers().getvr(xa);
		auto& dst = cpu.registers().getvr(xd);

		dst.df[0] = src_a.df[0] - src_j.df[0] * src_k.df[0];
		dst.df[1] = src_a.df[1] - src_j.df[1] * src_k.df[1];
		dst.df[2] = src_a.df[2] - src_j.df[2] * src_k.df[2];
		dst.df[3] = src_a.df[3] - src_j.df[3] * src_k.df[3];
	}

	static void XVFNMADD_S(cpu_t& cpu, la_instruction instr) {
		// XVFNMADD.S: LASX vector fused negative multiply-add (single precision, 8x32-bit)
		// 4R-type format: xd = -(xj * xk) + xa = xa - xj * xk
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;

		const auto& src_j = cpu.registers().getvr(xj);
		const auto& src_k = cpu.registers().getvr(xk);
		const auto& src_a = cpu.registers().getvr(xa);
		auto& dst = cpu.registers().getvr(xd);

		for (int i = 0; i < 8; i++) {
			dst.f[i] = src_a.f[i] - src_j.f[i] * src_k.f[i];
		}
	}

	static void XVFNMADD_D(cpu_t& cpu, la_instruction instr) {
		// XVFNMADD.D: LASX vector fused negative multiply-add (double precision, 4x64-bit)
		// 4R-type format: xd = -(xj * xk) + xa = xa - xj * xk
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;

		const auto& src_j = cpu.registers().getvr(xj);
		const auto& src_k = cpu.registers().getvr(xk);
		const auto& src_a = cpu.registers().getvr(xa);
		auto& dst = cpu.registers().getvr(xd);

		dst.df[0] = src_a.df[0] - src_j.df[0] * src_k.df[0];
		dst.df[1] = src_a.df[1] - src_j.df[1] * src_k.df[1];
		dst.df[2] = src_a.df[2] - src_j.df[2] * src_k.df[2];
		dst.df[3] = src_a.df[3] - src_j.df[3] * src_k.df[3];
	}

	static void XVFNMSUB_S(cpu_t& cpu, la_instruction instr) {
		// XVFNMSUB.S: LASX vector fused negative multiply-subtract (single precision, 8x32-bit)
		// 4R-type format: xd = -(xj * xk) - xa = -xa - xj * xk
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;

		const auto& src_j = cpu.registers().getvr(xj);
		const auto& src_k = cpu.registers().getvr(xk);
		const auto& src_a = cpu.registers().getvr(xa);
		auto& dst = cpu.registers().getvr(xd);

		for (int i = 0; i < 8; i++) {
			dst.f[i] = -(src_a.f[i] + src_j.f[i] * src_k.f[i]);
		}
	}

	static void XVFNMSUB_D(cpu_t& cpu, la_instruction instr) {
		// XVFNMSUB.D: LASX vector fused negative multiply-subtract (double precision, 4x64-bit)
		// 4R-type format: xd = -(xj * xk) - xa = -xa - xj * xk
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;

		const auto& src_j = cpu.registers().getvr(xj);
		const auto& src_k = cpu.registers().getvr(xk);
		const auto& src_a = cpu.registers().getvr(xa);
		auto& dst = cpu.registers().getvr(xd);

		dst.df[0] = -(src_a.df[0] + src_j.df[0] * src_k.df[0]);
		dst.df[1] = -(src_a.df[1] + src_j.df[1] * src_k.df[1]);
		dst.df[2] = -(src_a.df[2] + src_j.df[2] * src_k.df[2]);
		dst.df[3] = -(src_a.df[3] + src_j.df[3] * src_k.df[3]);
	}

	static void XVORI_B(cpu_t& cpu, la_instruction instr) {
		// XVORI.B xd, xj, ui8
		// Bitwise OR each byte of xj with immediate, store in xd
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t imm8 = (instr.whole >> 10) & 0xFF;

		const auto& src = cpu.registers().getvr(xj);
		auto& dst = cpu.registers().getvr(xd);

		// OR each byte with the immediate value
		uint64_t imm_broadcast = 0x0101010101010101ULL * imm8;
		dst.du[0] = src.du[0] | imm_broadcast;
		dst.du[1] = src.du[1] | imm_broadcast;
		dst.du[2] = src.du[2] | imm_broadcast;
		dst.du[3] = src.du[3] | imm_broadcast;
	}

	static void XVXORI_B(cpu_t& cpu, la_instruction instr) {
		// XVXORI.B xd, xj, ui8
		// Bitwise XOR each byte of xj with immediate, store in xd
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t imm8 = (instr.whole >> 10) & 0xFF;

		const auto& src = cpu.registers().getvr(xj);
		auto& dst = cpu.registers().getvr(xd);

		// XOR each byte with the immediate value
		uint64_t imm_broadcast = 0x0101010101010101ULL * imm8;
		dst.du[0] = src.du[0] ^ imm_broadcast;
		dst.du[1] = src.du[1] ^ imm_broadcast;
		dst.du[2] = src.du[2] ^ imm_broadcast;
		dst.du[3] = src.du[3] ^ imm_broadcast;
	}

	static void XVILVL_D(cpu_t& cpu, la_instruction instr) {
		// XVILVL.D: LASX vector interleave low double-word (256-bit)
		// Interleaves the low 128-bit double-words from two 256-bit vectors
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;

		const auto& src_j = cpu.registers().getvr(xj);
		const auto& src_k = cpu.registers().getvr(xk);
		auto& dst = cpu.registers().getvr(xd);

		// Interleave: dst[0] = src_k[0], dst[1] = src_j[0], dst[2] = src_k[1], dst[3] = src_j[1]
		// For LASX (256-bit), we interleave the low 2 double-words (128-bit) from each source
		dst.du[0] = src_k.du[0];
		dst.du[1] = src_j.du[0];
		dst.du[2] = src_k.du[1];
		dst.du[3] = src_j.du[1];
	}

	static void XVILVH_D(cpu_t& cpu, la_instruction instr) {
		// XVILVH.D: LASX vector interleave high double-word (256-bit)
		// Interleaves the high 128-bit double-words from two 256-bit vectors
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;

		const auto& src_j = cpu.registers().getvr(xj);
		const auto& src_k = cpu.registers().getvr(xk);
		auto& dst = cpu.registers().getvr(xd);

		// Interleave: dst[0] = src_k[2], dst[1] = src_j[2], dst[2] = src_k[3], dst[3] = src_j[3]
		// For LASX (256-bit), we interleave the high 2 double-words (128-bit) from each source
		dst.du[0] = src_k.du[2];
		dst.du[1] = src_j.du[2];
		dst.du[2] = src_k.du[3];
		dst.du[3] = src_j.du[3];
	}

	static void XVPERMI_D(cpu_t& cpu, la_instruction instr) {
		// XVPERMI.D: LASX vector permute double-word (256-bit)
		// Permutes 4 double-words based on 8-bit immediate
		// Each 2 bits of imm8 selects source element for corresponding dst element
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t imm8 = (instr.whole >> 10) & 0xFF;

		const auto& src = cpu.registers().getvr(xj);
		auto& dst = cpu.registers().getvr(xd);

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
	}

	static void XVPACKEV_D(cpu_t& cpu, la_instruction instr) {
		// XVPACKEV.D: LASX vector pack even double-word (256-bit)
		// Packs even-numbered elements (0, 2) from each source
		// dst[0] = xj[0], dst[1] = xk[0], dst[2] = xj[2], dst[3] = xk[2]
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;

		const auto& src_j = cpu.registers().getvr(xj);
		const auto& src_k = cpu.registers().getvr(xk);
		auto& dst = cpu.registers().getvr(xd);

		// Pack even elements (0 and 2) from both sources
		dst.du[0] = src_j.du[0];
		dst.du[1] = src_k.du[0];
		dst.du[2] = src_j.du[2];
		dst.du[3] = src_k.du[2];
	}

	static void XVPACKOD_D(cpu_t& cpu, la_instruction instr) {
		// XVPACKOD.D: LASX vector pack odd double-word (256-bit)
		// Packs odd-numbered elements (1, 3) from each source
		// dst[0] = xj[1], dst[1] = xk[1], dst[2] = xj[3], dst[3] = xk[3]
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;

		const auto& src_j = cpu.registers().getvr(xj);
		const auto& src_k = cpu.registers().getvr(xk);
		auto& dst = cpu.registers().getvr(xd);

		// Pack odd elements (1 and 3) from both sources
		dst.du[0] = src_j.du[1];
		dst.du[1] = src_k.du[1];
		dst.du[2] = src_j.du[3];
		dst.du[3] = src_k.du[3];
	}

	static void XVPICKEV_D(cpu_t& cpu, la_instruction instr) {
		// XVPICKEV.D: LASX vector pick even double-word (256-bit)
		// Picks even elements from both sources in a different pattern than PACKEV
		// dst[0] = xj[0], dst[1] = xj[2], dst[2] = xk[0], dst[3] = xk[2]
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;

		const auto& src_j = cpu.registers().getvr(xj);
		const auto& src_k = cpu.registers().getvr(xk);
		auto& dst = cpu.registers().getvr(xd);

		// Pick even elements (0 and 2) from both sources
		dst.du[0] = src_j.du[0];
		dst.du[1] = src_j.du[2];
		dst.du[2] = src_k.du[0];
		dst.du[3] = src_k.du[2];
	}

	static void XVPICKEV_W(cpu_t& cpu, la_instruction instr) {
		// XVPICKEV.W: LASX vector pick even word (256-bit)
		// Picks even-indexed words from both sources
		// dst[0] = xj[0], dst[1] = xj[2], dst[2] = xj[4], dst[3] = xj[6]
		// dst[4] = xk[0], dst[5] = xk[2], dst[6] = xk[4], dst[7] = xk[6]
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;

		const auto& src_j = cpu.registers().getvr(xj);
		const auto& src_k = cpu.registers().getvr(xk);
		auto& dst = cpu.registers().getvr(xd);

		// Pick even words (indices 0, 2, 4, 6) from both sources
		dst.w[0] = src_j.w[0];
		dst.w[1] = src_j.w[2];
		dst.w[2] = src_j.w[4];
		dst.w[3] = src_j.w[6];
		dst.w[4] = src_k.w[0];
		dst.w[5] = src_k.w[2];
		dst.w[6] = src_k.w[4];
		dst.w[7] = src_k.w[6];
	}

	static void XVPICKOD_D(cpu_t& cpu, la_instruction instr) {
		// XVPICKOD.D: LASX vector pick odd double-word (256-bit)
		// Picks odd elements from both sources
		// dst[0] = xj[1], dst[1] = xj[3], dst[2] = xk[1], dst[3] = xk[3]
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;

		const auto& src_j = cpu.registers().getvr(xj);
		const auto& src_k = cpu.registers().getvr(xk);
		auto& dst = cpu.registers().getvr(xd);

		// Pick odd elements (1 and 3) from both sources
		dst.du[0] = src_j.du[1];
		dst.du[1] = src_j.du[3];
		dst.du[2] = src_k.du[1];
		dst.du[3] = src_k.du[3];
	}

	static void XVLDI(cpu_t& cpu, la_instruction instr) {
		// XVLDI xd, imm13
		// LASX load immediate - loads immediate pattern into 256-bit vector
		// Format: bits[4:0] = xd, bits[17:5] = imm13
		// imm13 = [mode:3][value:10] where mode determines the pattern
		uint32_t xd = instr.whole & 0x1F;
		uint32_t imm13 = (instr.whole >> 5) & 0x1FFF;

		// Extract mode (top 3 bits) and value (bottom 10 bits)
		uint32_t mode = (imm13 >> 10) & 0x7;
		int32_t value = imm13 & 0x3FF;
		// Sign extend 10-bit value
		if (value & 0x200) value |= 0xFFFFFC00;

		auto& dst = cpu.registers().getvr(xd);

		// Mode 0-3: replicate byte/half-word/word/double-word
		if (mode == 0) {
			// Mode 0: replicate byte (8-bit)
			uint8_t byte_val = value & 0xFF;
			uint64_t pattern = 0;
			for (int i = 0; i < 8; i++) {
				pattern |= (uint64_t)byte_val << (i * 8);
			}
			dst.du[0] = dst.du[1] = dst.du[2] = dst.du[3] = pattern;
		} else if (mode == 1) {
			// Mode 1: replicate half-word (16-bit)
			uint16_t hword_val = value & 0xFFFF;
			uint64_t pattern = ((uint64_t)hword_val) | ((uint64_t)hword_val << 16) |
			                   ((uint64_t)hword_val << 32) | ((uint64_t)hword_val << 48);
			dst.du[0] = dst.du[1] = dst.du[2] = dst.du[3] = pattern;
		} else if (mode == 2) {
			// Mode 2: replicate word (32-bit)
			uint32_t word_val = value;
			uint64_t pattern = ((uint64_t)word_val) | ((uint64_t)word_val << 32);
			dst.du[0] = dst.du[1] = dst.du[2] = dst.du[3] = pattern;
		} else if (mode == 3) {
			// Mode 3: replicate double-word (64-bit)
			uint64_t dword_val = value;
			dst.du[0] = dst.du[1] = dst.du[2] = dst.du[3] = dword_val;
		} else {
			// Modes 4-7: reserved or special patterns, default to zero
			dst.du[0] = dst.du[1] = dst.du[2] = dst.du[3] = 0;
		}
	}

	static void INVALID(cpu_t& cpu, la_instruction instr) {
		cpu.trigger_exception(ILLEGAL_OPCODE, instr.whole);
	}

	static void UNIMPLEMENTED(cpu_t& cpu, la_instruction instr) {
		cpu.trigger_exception(UNIMPLEMENTED_INSTRUCTION, instr.whole);
	}

}; // InstrImpl

} // namespace loongarch
