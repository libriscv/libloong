#pragma once
#include "cpu.hpp"
#include "la_instr.hpp"
#include <cmath>

namespace loongarch {

template <int W>
struct InstrImpl {
	using cpu_t = CPU<W>;
	using addr_t = address_type<W>;
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
		int32_t result = (int32_t)cpu.reg(instr.ri12.rj) + InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		cpu.reg(instr.ri12.rd) = (int64_t)result;
	}

	static void ADDI_D(cpu_t& cpu, la_instruction instr) {
		cpu.reg(instr.ri12.rd) = cpu.reg(instr.ri12.rj) + InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
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
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		cpu.reg(instr.ri12.rd) = (int64_t)cpu.memory().template read<int8_t>(addr);
	}

	static void LD_H(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		cpu.reg(instr.ri12.rd) = (int64_t)cpu.memory().template read<int16_t>(addr);
	}

	static void LD_W(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		cpu.reg(instr.ri12.rd) = (int64_t)cpu.memory().template read<int32_t>(addr);
	}

	static void LD_D(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		cpu.reg(instr.ri12.rd) = cpu.memory().template read<int64_t>(addr);
	}

	static void LD_BU(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		cpu.reg(instr.ri12.rd) = (uint64_t)cpu.memory().template read<uint8_t>(addr);
	}

	static void LD_HU(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		cpu.reg(instr.ri12.rd) = (uint64_t)cpu.memory().template read<uint16_t>(addr);
	}

	static void LD_WU(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		cpu.reg(instr.ri12.rd) = (uint64_t)cpu.memory().template read<uint32_t>(addr);
	}

	static void ST_B(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		cpu.memory().template write<uint8_t>(addr, cpu.reg(instr.ri12.rd));
	}

	static void ST_H(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		cpu.memory().template write<uint16_t>(addr, cpu.reg(instr.ri12.rd));
	}

	static void ST_W(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		cpu.memory().template write<uint32_t>(addr, cpu.reg(instr.ri12.rd));
	}

	static void ST_D(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		cpu.memory().template write<uint64_t>(addr, cpu.reg(instr.ri12.rd));
	}

	static void LDPTR_W(cpu_t& cpu, la_instruction instr) {
		// LDPTR.W uses 14-bit signed offset << 2 (word-aligned)
		int64_t offset = InstructionHelpers<W>::sign_extend_14(instr.ri14.imm) << 2;
		auto addr = cpu.reg(instr.ri14.rj) + offset;
		// Sign-extend the 32-bit value to 64 bits
		cpu.reg(instr.ri14.rd) = (int64_t)(int32_t)cpu.memory().template read<uint32_t>(addr);
	}

	static void LDPTR_D(cpu_t& cpu, la_instruction instr) {
		// LDPTR.D uses 14-bit signed offset << 2 (word-aligned)
		int64_t offset = InstructionHelpers<W>::sign_extend_14(instr.ri14.imm) << 2;
		auto addr = cpu.reg(instr.ri14.rj) + offset;
		cpu.reg(instr.ri14.rd) = cpu.memory().template read<uint64_t>(addr);
	}

	static void STPTR_W(cpu_t& cpu, la_instruction instr) {
		// STPTR.W uses 14-bit signed offset << 2 (word-aligned)
		int64_t offset = InstructionHelpers<W>::sign_extend_14(instr.ri14.imm) << 2;
		auto addr = cpu.reg(instr.ri14.rj) + offset;
		cpu.memory().template write<uint32_t>(addr, cpu.reg(instr.ri14.rd));
	}

	static void STPTR_D(cpu_t& cpu, la_instruction instr) {
		// STPTR.D uses 14-bit signed offset << 2 (word-aligned)
		int64_t offset = InstructionHelpers<W>::sign_extend_14(instr.ri14.imm) << 2;
		auto addr = cpu.reg(instr.ri14.rj) + offset;
		cpu.memory().template write<uint64_t>(addr, cpu.reg(instr.ri14.rd));
	}

	// === Floating-point Load/Store Instructions ===

	static void FLD_S(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		uint32_t val = cpu.memory().template read<uint32_t>(addr);
		auto& vr = cpu.registers().getvr(instr.ri12.rd);
		vr.wu[0] = val;
		vr.wu[1] = 0;
		vr.du[1] = 0;
	}

	static void FST_S(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		const auto& vr = cpu.registers().getvr(instr.ri12.rd);
		cpu.memory().template write<uint32_t>(addr, vr.wu[0]);
	}

	static void FLD_D(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		uint64_t val = cpu.memory().template read<uint64_t>(addr);
		auto& vr = cpu.registers().getvr(instr.ri12.rd);
		vr.du[0] = val;
		vr.du[1] = 0;
	}

	static void FST_D(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		const auto& vr = cpu.registers().getvr(instr.ri12.rd);
		cpu.memory().template write<uint64_t>(addr, vr.du[0]);
	}

	// === Indexed Load/Store Instructions ===

	static void STX_B(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		cpu.memory().template write<uint8_t>(addr, cpu.reg(instr.r3.rd));
	}

	static void STX_H(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		cpu.memory().template write<uint16_t>(addr, cpu.reg(instr.r3.rd));
	}

	static void STX_W(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		cpu.memory().template write<uint32_t>(addr, cpu.reg(instr.r3.rd));
	}

	static void STX_D(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		cpu.memory().template write<uint64_t>(addr, cpu.reg(instr.r3.rd));
	}

	static void FLDX_D(cpu_t& cpu, la_instruction instr) {
		// Floating-point indexed load (double precision)
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		auto& vr = cpu.registers().getvr(instr.r3.rd);
		vr.du[0] = cpu.memory().template read<uint64_t>(addr);
	}

	static void FSTX_D(cpu_t& cpu, la_instruction instr) {
		// Floating-point indexed store (double precision)
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		const auto& vr = cpu.registers().getvr(instr.r3.rd);
		cpu.memory().template write<uint64_t>(addr, vr.du[0]);
	}

	static void VLDX(cpu_t& cpu, la_instruction instr) {
		// Vector indexed load (LSX 128-bit)
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		auto& vr = cpu.registers().getvr(instr.r3.rd);
		vr.du[0] = cpu.memory().template read<uint64_t>(addr);
		vr.du[1] = cpu.memory().template read<uint64_t>(addr + 8);
	}

	static void VSTX(cpu_t& cpu, la_instruction instr) {
		// Vector indexed store (LSX 128-bit)
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		const auto& vr = cpu.registers().getvr(instr.r3.rd);
		cpu.memory().template write<uint64_t>(addr, vr.du[0]);
		cpu.memory().template write<uint64_t>(addr + 8, vr.du[1]);
	}

	// === Branch Instructions ===

	static void BEQZ(cpu_t& cpu, la_instruction instr) {
		// BEQZ uses ri21 format: rj at bits[9:5], 21-bit offset split across bits[25:10] and [4:0]
		if (cpu.reg(instr.ri21.rj) == 0) {
			auto offset = InstructionHelpers<W>::sign_extend_21(instr.ri21.offs_lo, instr.ri21.offs_hi) << 2;
			cpu.increment_pc(offset-4);
		}
	}

	static void BNEZ(cpu_t& cpu, la_instruction instr) {
		// BNEZ uses ri21 format: rj at bits[9:5], 21-bit offset split across bits[25:10] and [4:0]
		if (cpu.reg(instr.ri21.rj) != 0) {
			auto offset = InstructionHelpers<W>::sign_extend_21(instr.ri21.offs_lo, instr.ri21.offs_hi) << 2;
			cpu.increment_pc(offset-4);
		}
	}

	static void BEQ(cpu_t& cpu, la_instruction instr) {
		if (cpu.reg(instr.ri16.rj) == cpu.reg(instr.ri16.rd)) {
			auto offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
			cpu.increment_pc(offset-4);
		}
	}

	static void BNE(cpu_t& cpu, la_instruction instr) {
		if (cpu.reg(instr.ri16.rj) != cpu.reg(instr.ri16.rd)) {
			auto offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
			cpu.increment_pc(offset-4);
		}
	}

	static void BLT(cpu_t& cpu, la_instruction instr) {
		if ((int64_t)cpu.reg(instr.ri16.rj) < (int64_t)cpu.reg(instr.ri16.rd)) {
			auto offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
			cpu.increment_pc(offset-4);
		}
	}

	static void BGE(cpu_t& cpu, la_instruction instr) {
		if ((int64_t)cpu.reg(instr.ri16.rj) >= (int64_t)cpu.reg(instr.ri16.rd)) {
			auto offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
			cpu.increment_pc(offset-4);
		}
	}

	static void BLTU(cpu_t& cpu, la_instruction instr) {
		if ((uint64_t)cpu.reg(instr.ri16.rj) < (uint64_t)cpu.reg(instr.ri16.rd)) {
			auto offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
			cpu.increment_pc(offset-4);
		}
	}

	static void BGEU(cpu_t& cpu, la_instruction instr) {
		if ((uint64_t)cpu.reg(instr.ri16.rj) >= (uint64_t)cpu.reg(instr.ri16.rd)) {
			auto offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
			cpu.increment_pc(offset-4);
		}
	}

	static void B(cpu_t& cpu, la_instruction instr) {
		auto offset = InstructionHelpers<W>::sign_extend_26(instr.i26.offs()) << 2;
		cpu.increment_pc(offset-4);
	}

	static void BL(cpu_t& cpu, la_instruction instr) {
		cpu.reg(REG_RA) = cpu.pc() + 4;
		auto offset = InstructionHelpers<W>::sign_extend_26(instr.i26.offs()) << 2;
		cpu.increment_pc(offset-4);
	}

	static void JIRL(cpu_t& cpu, la_instruction instr) {
		auto next_pc = cpu.pc() + 4;
		auto base = cpu.reg(instr.ri16.rj);
		auto offset = InstructionHelpers<W>::sign_extend_16(instr.ri16.imm) << 2;
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
		uint64_t lower = cpu.reg(instr.ri20.rd) & 0xFFFFFFFF;

		// Sign-extend the 20-bit immediate to 32 bits, then place at bits [51:32]
		int32_t si20 = InstructionHelpers<W>::sign_extend_20(instr.ri20.imm);
		uint64_t imm_ext = ((uint64_t)(uint32_t)si20) << 32;

		cpu.reg(instr.ri20.rd) = imm_ext | lower;
	}

	static void PCADDI(cpu_t& cpu, la_instruction instr) {
		int32_t si20 = InstructionHelpers<W>::sign_extend_20(instr.ri20.imm);
		int64_t offset = (int64_t)(si20 << 2);
		cpu.reg(instr.ri20.rd) = cpu.pc() + offset;
	}

	static void PCADDU12I(cpu_t& cpu, la_instruction instr) {
		int32_t si20 = InstructionHelpers<W>::sign_extend_20(instr.ri20.imm);
		int64_t offset = (int64_t)(si20 << 12);
		cpu.reg(instr.ri20.rd) = cpu.pc() + offset;
	}

	static void PCALAU12I(cpu_t& cpu, la_instruction instr) {
		auto pc_aligned = cpu.pc() & ~((addr_t)0xFFF);
		int64_t offset = (int64_t)(int32_t)(instr.ri20.imm << 12);
		cpu.reg(instr.ri20.rd) = pc_aligned + offset;
	}

	static void LU52I_D(cpu_t& cpu, la_instruction instr) {
		uint64_t base = cpu.reg(instr.ri12.rj) & 0x000FFFFFFFFFFFFF;
		uint64_t upper = ((uint64_t)instr.ri12.imm) << 52;
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
		auto addr = cpu.reg(instr.ri14.rj) + (InstructionHelpers<W>::sign_extend_14(instr.ri14.imm) << 2);
		cpu.reg(instr.ri14.rd) = (int64_t)(int32_t)cpu.memory().template read<uint32_t>(addr);
		// In single-threaded mode, we always succeed
		cpu.set_ll_bit(true);
	}

	static void LL_D(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri14.rj) + (InstructionHelpers<W>::sign_extend_14(instr.ri14.imm) << 2);
		cpu.reg(instr.ri14.rd) = cpu.memory().template read<uint64_t>(addr);
		cpu.set_ll_bit(true);
	}

	static void SC_W(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri14.rj) + (InstructionHelpers<W>::sign_extend_14(instr.ri14.imm) << 2);
		if (cpu.ll_bit()) {
			cpu.memory().template write<uint32_t>(addr, cpu.reg(instr.ri14.rd));
			cpu.reg(instr.ri14.rd) = 1; // Success
		} else {
			cpu.reg(instr.ri14.rd) = 0; // Failure
		}
		cpu.set_ll_bit(false);
	}

	static void SC_D(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.ri14.rj) + (InstructionHelpers<W>::sign_extend_14(instr.ri14.imm) << 2);
		if (cpu.ll_bit()) {
			cpu.memory().template write<uint64_t>(addr, cpu.reg(instr.ri14.rd));
			cpu.reg(instr.ri14.rd) = 1; // Success
		} else {
			cpu.reg(instr.ri14.rd) = 0; // Failure
		}
		cpu.set_ll_bit(false);
	}

	// === Indexed Load Instructions ===

	static void LDX_B(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		cpu.reg(instr.r3.rd) = (int64_t)cpu.memory().template read<int8_t>(addr);
	}

	static void LDX_H(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		cpu.reg(instr.r3.rd) = (int64_t)cpu.memory().template read<int16_t>(addr);
	}

	static void LDX_W(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		cpu.reg(instr.r3.rd) = (int64_t)cpu.memory().template read<int32_t>(addr);
	}

	static void LDX_D(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		cpu.reg(instr.r3.rd) = cpu.memory().template read<int64_t>(addr);
	}

	static void LDX_BU(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		cpu.reg(instr.r3.rd) = (uint64_t)cpu.memory().template read<uint8_t>(addr);
	}

	static void LDX_HU(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		cpu.reg(instr.r3.rd) = (uint64_t)cpu.memory().template read<uint16_t>(addr);
	}

	static void LDX_WU(cpu_t& cpu, la_instruction instr) {
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		cpu.reg(instr.r3.rd) = (uint64_t)cpu.memory().template read<uint32_t>(addr);
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
		int64_t b = InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		cpu.reg(instr.ri12.rd) = (a < b) ? 1 : 0;
	}

	static void SLTUI(cpu_t& cpu, la_instruction instr) {
		uint64_t a = cpu.reg(instr.ri12.rj);
		uint64_t b = static_cast<uint64_t>(InstructionHelpers<W>::sign_extend_12(instr.ri12.imm));
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

	static void FCMP_COR_D(cpu_t& cpu, la_instruction instr) {
		// Floating-point compare ordered (checks if neither operand is NaN)
		uint32_t cd = instr.whole & 0x7;  // FCC register index (3 bits)
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		uint32_t fk = (instr.whole >> 10) & 0x1F;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		double fj_val = vr_j.df[0];
		double fk_val = vr_k.df[0];

		// COR (Comparison Ordered): true if neither operand is NaN
		bool result = !std::isnan(fj_val) && !std::isnan(fk_val);
		cpu.registers().set_cf(cd, result ? 1 : 0);
	}

	static void FCMP_CULE_D(cpu_t& cpu, la_instruction instr) {
		// Floating-point compare unordered or less than or equal
		uint32_t cd = instr.whole & 0x7;  // FCC register index (3 bits)
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		uint32_t fk = (instr.whole >> 10) & 0x1F;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		double fj_val = vr_j.df[0];
		double fk_val = vr_k.df[0];

		// CULE (Comparison Unordered or Less than or Equal): true if unordered (NaN) or fj <= fk
		bool result = std::isnan(fj_val) || std::isnan(fk_val) || (fj_val <= fk_val);
		cpu.registers().set_cf(cd, result ? 1 : 0);
	}

	static void FCMP_SLT_D(cpu_t& cpu, la_instruction instr) {
		// Floating-point compare signed less than (ordered)
		uint32_t cd = instr.whole & 0x7;  // FCC register index (3 bits)
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		uint32_t fk = (instr.whole >> 10) & 0x1F;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		double fj_val = vr_j.df[0];
		double fk_val = vr_k.df[0];

		// SLT (Signed Less Than): true if both are ordered and fj < fk
		bool result = !std::isnan(fj_val) && !std::isnan(fk_val) && (fj_val < fk_val);
		cpu.registers().set_cf(cd, result ? 1 : 0);
	}

	static void VFCMP_SLT_D(cpu_t& cpu, la_instruction instr) {
		// Vector floating-point compare signed less than (double)
		// Compares each double-precision element and sets result mask
		// LoongArch semantics: sets all bits to 1 when condition is TRUE, all 0s when FALSE
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		// For each double element: if src1[i] < src2[i] is TRUE, set to all 1s; if FALSE, set to 0
		for (int i = 0; i < 2; i++) {
			double val1 = src1.df[i];
			double val2 = src2.df[i];
			bool cmp = !std::isnan(val1) && !std::isnan(val2) && (val1 < val2);
			dst.du[i] = cmp ? 0xFFFFFFFFFFFFFFFFULL : 0;
		}
	}

	static void FCMP_SLE_D(cpu_t& cpu, la_instruction instr) {
		// Floating-point compare signed less-or-equal (ordered)
		uint32_t cd = instr.whole & 0x7;  // FCC register index (3 bits)
		uint32_t fj = (instr.whole >> 5) & 0x1F;
		uint32_t fk = (instr.whole >> 10) & 0x1F;

		const auto& vr_j = cpu.registers().getvr(fj);
		const auto& vr_k = cpu.registers().getvr(fk);
		double fj_val = vr_j.df[0];
		double fk_val = vr_k.df[0];

		// SLE (Signed Less-or-Equal): true if both are ordered and fj <= fk
		bool result = !std::isnan(fj_val) && !std::isnan(fk_val) && (fj_val <= fk_val);
		cpu.registers().set_cf(cd, result ? 1 : 0);
	}

	static void VFCMP_SLE_D(cpu_t& cpu, la_instruction instr) {
		// Vector floating-point compare signed less-or-equal (double)
		// Compares each double-precision element and sets result mask
		// LoongArch semantics: sets all bits to 1 when condition is TRUE, all 0s when FALSE
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		// For each double element: if src1[i] <= src2[i] is TRUE, set to all 1s; if FALSE, set to 0
		for (int i = 0; i < 2; i++) {
			double val1 = src1.df[i];
			double val2 = src2.df[i];
			bool cmp = !std::isnan(val1) && !std::isnan(val2) && (val1 <= val2);
			dst.du[i] = cmp ? 0xFFFFFFFFFFFFFFFFULL : 0;
		}
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

	static void CLO_W(cpu_t& cpu, la_instruction instr) {
		uint32_t val = static_cast<uint32_t>(cpu.reg(instr.r2.rj));
		int count = 0;
		for (int i = 31; i >= 0 && (val & (1u << i)); i--) count++;
		cpu.reg(instr.r2.rd) = count;
	}

	static void CLZ_W(cpu_t& cpu, la_instruction instr) {
		uint32_t val = static_cast<uint32_t>(cpu.reg(instr.r2.rj));
		cpu.reg(instr.r2.rd) = val ? __builtin_clz(val) : 32;
	}

	static void CTO_W(cpu_t& cpu, la_instruction instr) {
		uint32_t val = static_cast<uint32_t>(cpu.reg(instr.r2.rj));
		int count = 0;
		for (int i = 0; i < 32 && (val & (1u << i)); i++) count++;
		cpu.reg(instr.r2.rd) = count;
	}

	static void CTZ_W(cpu_t& cpu, la_instruction instr) {
		uint32_t val = static_cast<uint32_t>(cpu.reg(instr.r2.rj));
		cpu.reg(instr.r2.rd) = val ? __builtin_ctz(val) : 32;
	}

	static void CLO_D(cpu_t& cpu, la_instruction instr) {
		uint64_t val = cpu.reg(instr.r2.rj);
		int count = 0;
		for (int i = 63; i >= 0 && (val & (1ULL << i)); i--) count++;
		cpu.reg(instr.r2.rd) = count;
	}

	static void CLZ_D(cpu_t& cpu, la_instruction instr) {
		uint64_t val = cpu.reg(instr.r2.rj);
		cpu.reg(instr.r2.rd) = val ? __builtin_clzll(val) : 64;
	}

	static void CTO_D(cpu_t& cpu, la_instruction instr) {
		uint64_t val = cpu.reg(instr.r2.rj);
		int count = 0;
		for (int i = 0; i < 64 && (val & (1ULL << i)); i++) count++;
		cpu.reg(instr.r2.rd) = count;
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
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		auto& vr = cpu.registers().getvr(instr.ri12.rd);
		vr.du[0] = cpu.memory().template read<uint64_t>(addr);
		vr.du[1] = cpu.memory().template read<uint64_t>(addr + 8);
	}

	static void VST(cpu_t& cpu, la_instruction instr) {
		// VST vd, rj, si12
		// Store 128-bit vector to memory
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		const auto& vr = cpu.registers().getvr(instr.ri12.rd);
		cpu.memory().template write<uint64_t>(addr, vr.du[0]);
		cpu.memory().template write<uint64_t>(addr + 8, vr.du[1]);
	}

	static void XVLD(cpu_t& cpu, la_instruction instr) {
		// XVLD xd, rj, si12
		// Load 256-bit LASX vector from memory
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		auto& vr = cpu.registers().getvr(instr.ri12.rd);
		vr.du[0] = cpu.memory().template read<uint64_t>(addr);
		vr.du[1] = cpu.memory().template read<uint64_t>(addr + 8);
		vr.du[2] = cpu.memory().template read<uint64_t>(addr + 16);
		vr.du[3] = cpu.memory().template read<uint64_t>(addr + 24);
	}

	static void XVST(cpu_t& cpu, la_instruction instr) {
		// XVST xd, rj, si12
		// Store 256-bit LASX vector to memory
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers<W>::sign_extend_12(instr.ri12.imm);
		const auto& vr = cpu.registers().getvr(instr.ri12.rd);
		cpu.memory().template write<uint64_t>(addr, vr.du[0]);
		cpu.memory().template write<uint64_t>(addr + 8, vr.du[1]);
		cpu.memory().template write<uint64_t>(addr + 16, vr.du[2]);
		cpu.memory().template write<uint64_t>(addr + 24, vr.du[3]);
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
		dst.d[0] = (int64_t)src1.w[0] + (int64_t)src1.w[1];
		dst.d[1] = (int64_t)src2.w[0] + (int64_t)src2.w[1];
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

	static void VPICKVE2GR_BU(cpu_t& cpu, la_instruction instr) {
		// VPICKVE2GR.BU rd, vj, ui4
		// Pick unsigned byte element from vector to general register (zero-extended)
		uint32_t rd = instr.whole & 0x1F;
		if (rd == 0) return; // Writes to x0 are discarded
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t ui4 = (instr.whole >> 10) & 0xF;

		const auto& src = cpu.registers().getvr(vj);
		cpu.reg(rd) = src.bu[ui4];
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

		for (int i = 0; i < 2; i++) {
			uint64_t result = 0;
			for (int j = 0; j < 8; j++) {
				uint8_t b1 = (src1.du[i] >> (j * 8)) & 0xFF;
				uint8_t b2 = (src2.du[i] >> (j * 8)) & 0xFF;
				uint8_t sum = b1 + b2;
				result |= (uint64_t)sum << (j * 8);
			}
			dst.du[i] = result;
		}
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
	}

	static void VMIN_BU(cpu_t& cpu, la_instruction instr) {
		// VMIN.BU vd, vj, vk
		// Unsigned minimum of corresponding bytes
		uint32_t vd = instr.whole & 0x1F;
		uint32_t vj = (instr.whole >> 5) & 0x1F;
		uint32_t vk = (instr.whole >> 10) & 0x1F;

		const auto& src1 = cpu.registers().getvr(vj);
		const auto& src2 = cpu.registers().getvr(vk);
		auto& dst = cpu.registers().getvr(vd);

		for (int i = 0; i < 2; i++) {
			uint64_t result = 0;
			for (int j = 0; j < 8; j++) {
				uint8_t b1 = (src1.du[i] >> (j * 8)) & 0xFF;
				uint8_t b2 = (src2.du[i] >> (j * 8)) & 0xFF;
				uint8_t minv = (b1 < b2) ? b1 : b2;
				result |= (uint64_t)minv << (j * 8);
			}
			dst.du[i] = result;
		}
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

	static void XVMIN_BU(cpu_t& cpu, la_instruction instr) {
		// XVMIN.BU xd, xj, xk
		// Unsigned minimum of corresponding bytes (256-bit)
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
				uint8_t min_val = (b1 < b2) ? b1 : b2;
				result |= (uint64_t)min_val << (j * 8);
			}
			dst.du[i] = result;
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

		for (int i = 0; i < 4; i++) {
			uint64_t result = 0;
			for (int j = 0; j < 8; j++) {
				uint8_t b1 = (src1.du[i] >> (j * 8)) & 0xFF;
				uint8_t b2 = (src2.du[i] >> (j * 8)) & 0xFF;
				uint8_t max_val = (b1 > b2) ? b1 : b2;
				result |= (uint64_t)max_val << (j * 8);
			}
			dst.du[i] = result;
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
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t imm = (instr.whole >> 10) & 0xFF;

		const auto& src = cpu.registers().getvr(xj);
		auto& dst = cpu.registers().getvr(xd);

		// imm specifies which 128-bit chunks to select
		// Bits [1:0] select source for low 128 bits
		// Bits [3:2] select source for high 128 bits
		uint32_t lo_sel = (imm >> 0) & 0x3;
		uint32_t hi_sel = (imm >> 2) & 0x3;

		uint64_t tmp[4];
		tmp[0] = src.du[0];
		tmp[1] = src.du[1];
		tmp[2] = src.du[2];
		tmp[3] = src.du[3];

		dst.du[0] = tmp[lo_sel * 2];
		dst.du[1] = tmp[lo_sel * 2 + 1];
		dst.du[2] = tmp[hi_sel * 2];
		dst.du[3] = tmp[hi_sel * 2 + 1];
	}

	static void UNIMPLEMENTED(cpu_t& cpu, la_instruction instr) {
		fprintf(stderr, "UNIMPLEMENTED at PC=0x%lx: 0x%08x\n",
			(unsigned long)cpu.pc(), instr.whole);
		cpu.trigger_exception(UNIMPLEMENTED_INSTRUCTION, instr.whole);
	}

}; // InstrImpl

} // namespace loongarch
