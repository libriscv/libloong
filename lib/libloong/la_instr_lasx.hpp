#pragma once
// LASX (Loongson Advanced SIMD eXtension) 256-bit vector instructions
// This file contains LASX instruction implementations and printers
// It should be included with #ifdef LA_LASX_ENABLED guard

#include "cpu.hpp"
#include "la_instr.hpp"
#include <cmath>
#include <cstdio>

namespace loongarch {

// LASX instruction implementations
struct InstrImplLASX {
	using cpu_t = CPU;
	using addr_t = address_t;
	using saddr_t = std::make_signed_t<addr_t>;

	static void XVLD(cpu_t& cpu, la_instruction instr) {
		// XVLD xd, rj, si12
		// Load 256-bit LASX vector from memory
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
		auto& vr = cpu.registers().getvr(instr.ri12.rd);
		vr = cpu.memory().template read<remove_cvref_t<decltype(vr)>, true>(addr);
	}

	static void XVST(cpu_t& cpu, la_instruction instr) {
		// XVST xd, rj, si12
		// Store 256-bit LASX vector to memory
		auto addr = cpu.reg(instr.ri12.rj) + InstructionHelpers::sign_extend_12(instr.ri12.imm);
		const auto& vr = cpu.registers().getvr(instr.ri12.rd);
		cpu.memory().template write<remove_cvref_t<decltype(vr)>, true>(addr, vr);
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
		if (instr.r2.rd == 0) return; // Writes to x0 are discarded
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t ui3 = (instr.whole >> 10) & 0x7; // 3-bit index for 8 words

		const auto& src = cpu.registers().getvr(xj);
		cpu.reg(instr.r2.rd) = static_cast<int64_t>(static_cast<int32_t>(src.wu[ui3]));
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
						dst.du[i] = (val1 < val2) ? UINT64_MAX : 0;
					}
					break;
				case 0x04: // CEQ - Equal (ordered)
				case 0x05: // SEQ - Signaling Equal (ordered)
					if (std::isnan(val1) || std::isnan(val2)) {
						dst.du[i] = 0;
					} else {
						dst.du[i] = (val1 == val2) ? UINT64_MAX : 0;
					}
					break;
				case 0x06: // CLE - (Quiet) Less or Equal (ordered)
				case 0x07: // SLE - Signaling Less or Equal (ordered)
					if (std::isnan(val1) || std::isnan(val2)) {
						dst.du[i] = 0;
					} else {
						dst.du[i] = (val1 <= val2) ? UINT64_MAX : 0;
					}
					break;
				case 0x0E: // CULE - (Quiet) Unordered or Less or Equal
				case 0x0F: // SULE - Signaling Unordered or Less or Equal
					if (std::isnan(val1) || std::isnan(val2)) {
						dst.du[i] = UINT64_MAX;
					} else {
						dst.du[i] = (val1 <= val2) ? UINT64_MAX : 0;
					}
					break;
				case 0x14: // COR - (Quiet) Ordered
					if (std::isnan(val1) || std::isnan(val2)) {
						dst.du[i] = 0;
					} else {
						dst.du[i] = UINT64_MAX;
					}
					break;
				case 0x18: // CUNE - (Quiet) Unordered or Not Equal
				case 0x19: // SUNE - Signaling Unordered or Not Equal
					if (std::isnan(val1) || std::isnan(val2)) {
						dst.du[i] = UINT64_MAX;
					} else {
						dst.du[i] = (val1 != val2) ? UINT64_MAX : 0;
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
		__int128 q0 = (__int128)src1.d[0] + (__int128)src1.d[1];
		__int128 q1 = (__int128)src1.d[2] + (__int128)src1.d[3];

		// Store quadwords as pairs of doublewords
		auto& dst = cpu.registers().getvr(xd);
		dst.d[0] = (int64_t)q0;
		dst.d[1] = (int64_t)(q0 >> 64);
		dst.d[2] = (int64_t)q1;
		dst.d[3] = (int64_t)(q1 >> 64);
	}

	static void XVREPLGR2VR_B(cpu_t& cpu, la_instruction instr) {
		// XVREPLGR2VR.B xd, rj
		// Replicate byte from GPR rj to all 32 bytes of xd
		uint32_t xd = instr.whole & 0x1F;

		uint8_t value = cpu.reg(instr.r2.rj) & 0xFF;
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
		vr = cpu.memory().template read<remove_cvref_t<decltype(vr)>, true>(addr);
	}

	static void XVSTX(cpu_t& cpu, la_instruction instr) {
		// XVSTX xd, rj, rk
		// Vector indexed store (LASX 256-bit)
		auto addr = cpu.reg(instr.r3.rj) + cpu.reg(instr.r3.rk);
		const auto& vr = cpu.registers().getvr(instr.r3.rd);
		cpu.memory().template write<remove_cvref_t<decltype(vr)>, true>(addr, vr);
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
		uint32_t xd = instr.whole & 0x1F;
		uint32_t imm13 = (instr.whole >> 5) & 0x1FFF;

		auto& dst = cpu.registers().getvr(xd);

		// Extract top bits to determine mode
		uint32_t top3 = (imm13 >> 10) & 0x7;  // imm[12:10]
		uint32_t top5 = (imm13 >> 8) & 0x1F;  // imm[12:8]
		uint32_t imm8 = imm13 & 0xFF;         // imm[7:0]
		uint32_t imm10 = imm13 & 0x3FF;       // imm[9:0]

		// Sign-extend imm10 from 10 bits to 64 bits
		int64_t sext_imm10 = (int64_t)(int16_t)(imm10 << 6) >> 6;

		// Pattern based on specification (same as VLDI but for 256-bit vectors)
		if (top3 == 0b000) {
			// imm[12:10]=0b000: broadcast imm[7:0] as 8-bit elements
			for (int i = 0; i < 32; i++) dst.bu[i] = (uint8_t)imm8;
		} else if (top3 == 0b001) {
			// imm[12:10]=0b001: broadcast sign-extended imm[9:0] as 16-bit elements
			for (int i = 0; i < 16; i++) dst.hu[i] = (uint16_t)sext_imm10;
		} else if (top3 == 0b010) {
			// imm[12:10]=0b010: broadcast sign-extended imm[9:0] as 32-bit elements
			for (int i = 0; i < 8; i++) dst.wu[i] = (uint32_t)sext_imm10;
		} else if (top3 == 0b011) {
			// imm[12:10]=0b011: broadcast sign-extended imm[9:0] as 64-bit elements
			for (int i = 0; i < 4; i++) dst.du[i] = (uint64_t)sext_imm10;
		} else if (top5 == 0b10000) {
			// imm[12:8]=0b10000: broadcast imm[7:0] as 32-bit elements
			uint32_t val = imm8;
			for (int i = 0; i < 8; i++) dst.wu[i] = val;
		} else if (top5 == 0b10001) {
			// imm[12:8]=0b10001: broadcast imm[7:0] << 8 as 32-bit elements
			uint32_t val = imm8 << 8;
			for (int i = 0; i < 8; i++) dst.wu[i] = val;
		} else if (top5 == 0b10010) {
			// imm[12:8]=0b10010: broadcast imm[7:0] << 16 as 32-bit elements
			uint32_t val = imm8 << 16;
			for (int i = 0; i < 8; i++) dst.wu[i] = val;
		} else if (top5 == 0b10011) {
			// imm[12:8]=0b10011: broadcast imm[7:0] << 24 as 32-bit elements
			uint32_t val = imm8 << 24;
			for (int i = 0; i < 8; i++) dst.wu[i] = val;
		} else if (top5 == 0b10100) {
			// imm[12:8]=0b10100: broadcast imm[7:0] as 16-bit elements
			uint16_t val = (uint16_t)imm8;
			for (int i = 0; i < 16; i++) dst.hu[i] = val;
		} else if (top5 == 0b10101) {
			// imm[12:8]=0b10101: broadcast imm[7:0] << 8 as 16-bit elements
			uint16_t val = (uint16_t)(imm8 << 8);
			for (int i = 0; i < 16; i++) dst.hu[i] = val;
		} else if (top5 == 0b10110) {
			// imm[12:8]=0b10110: broadcast (imm[7:0] << 8) | 0xFF as 32-bit elements
			uint32_t val = (imm8 << 8) | 0xFF;
			for (int i = 0; i < 8; i++) dst.wu[i] = val;
		} else if (top5 == 0b10111) {
			// imm[12:8]=0b10111: broadcast (imm[7:0] << 16) | 0xFFFF as 32-bit elements
			uint32_t val = (imm8 << 16) | 0xFFFF;
			for (int i = 0; i < 8; i++) dst.wu[i] = val;
		} else if (top5 == 0b11000) {
			// imm[12:8]=0b11000: broadcast imm[7:0] as 8-bit elements (duplicate of 0b000 case)
			for (int i = 0; i < 32; i++) dst.bu[i] = (uint8_t)imm8;
		} else if (top5 == 0b11001) {
			// imm[12:8]=0b11001: repeat each bit of imm[7:0] eight times, broadcast as 64-bit elements
			uint64_t val = 0;
			for (int bit = 0; bit < 8; bit++) {
				if (imm8 & (1 << bit)) {
					val |= (0xFFULL << (bit * 8));
				}
			}
			for (int i = 0; i < 4; i++) dst.du[i] = val;
		} else if (top5 == 0b11010) {
			// imm[12:8]=0b11010: broadcast specific pattern as 32-bit elements
			// (imm[7] << 31) | ((1-imm[6]) << 30) | ((imm[6] * 0x1F) << 25) | (imm[5:0] << 19)
			uint32_t bit7 = (imm8 >> 7) & 1;
			uint32_t bit6 = (imm8 >> 6) & 1;
			uint32_t bits5_0 = imm8 & 0x3F;
			uint32_t val = (bit7 << 31) | ((1 - bit6) << 30) | ((bit6 * 0x1F) << 25) | (bits5_0 << 19);
			for (int i = 0; i < 8; i++) dst.wu[i] = val;
		} else if (top5 == 0b11011) {
			// imm[12:8]=0b11011: broadcast specific pattern as 64-bit elements
			// (imm[7] << 31) | ((1-imm[6]) << 30) | ((imm[6] * 0x1F) << 25) | (imm[5:0] << 19)
			uint32_t bit7 = (imm8 >> 7) & 1;
			uint32_t bit6 = (imm8 >> 6) & 1;
			uint32_t bits5_0 = imm8 & 0x3F;
			uint64_t val = (uint64_t)((bit7 << 31) | ((1 - bit6) << 30) | ((bit6 * 0x1F) << 25) | (bits5_0 << 19));
			for (int i = 0; i < 4; i++) dst.du[i] = val;
		} else if (top5 == 0b11100) {
			// imm[12:8]=0b11100: broadcast specific pattern as 64-bit elements
			// (imm[7] << 63) | ((1-imm[6]) << 62) | ((imm[6] * 0xFF) << 54) | (imm[5:0] << 48)
			uint64_t bit7 = (imm8 >> 7) & 1;
			uint64_t bit6 = (imm8 >> 6) & 1;
			uint64_t bits5_0 = imm8 & 0x3F;
			uint64_t val = (bit7 << 63) | ((1 - bit6) << 62) | ((bit6 * 0xFF) << 54) | (bits5_0 << 48);
			for (int i = 0; i < 4; i++) dst.du[i] = val;
		} else {
			throw MachineException(ILLEGAL_OPCODE,
				"XVLDI: Unknown mode", top5);
		}
	}

}; // InstrImplLASX

// LASX instruction printers for debugging
struct InstrPrintersLASX {
	using cpu_t = CPU;
	using addr_t = address_t;

	static int XVLD(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "xvld $xr%u, %s, %d",
			instr.ri12.rd, reg_name(instr.ri12.rj), imm);
	}

	static int XVST(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		int32_t imm = InstructionHelpers::sign_extend_12(instr.ri12.imm);
		return snprintf(buf, len, "xvst $xr%u, %s, %d",
			instr.ri12.rd, reg_name(instr.ri12.rj), imm);
	}

	static int XVHADDW_D_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvhaddw.d.w $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVPICKVE2GR_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t ui3 = (instr.whole >> 10) & 0x7;
		return snprintf(buf, len, "xvpickve2gr.w %s, $xr%u, %u", reg_name(instr.r2.rd), xj, ui3);
	}

	static int XVADD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvadd.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVBITSEL_V(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "xvbitsel.v $xr%u, $xr%u, $xr%u, $xr%u", xd, xj, xk, xa);
	}

	static int XVFCMP_COND_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t cond = (instr.whole >> 15) & 0x1F;
		const char* mnemonic = cond_names(cond);
		return snprintf(buf, len, "xvfcmp.%s.d $xr%u, $xr%u, $xr%u", mnemonic, xd, xj, xk);
	}

	static int XVHADDW_Q_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvhaddw.q.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVREPLGR2VR_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		return snprintf(buf, len, "xvreplgr2vr.b $xr%u, %s", xd, reg_name(instr.r2.rj));
	}

	static int XVXOR_V(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvxor.v $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVSUB_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvsub.w $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVMIN_BU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvmin.bu $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVMAX_BU(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvmax.bu $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVMSKNZ_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "xvmsknz.b $xr%u, $xr%u", xd, xj);
	}

	static int XVPICKVE_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t idx = (instr.whole >> 10) & 0x7;
		return snprintf(buf, len, "xvpickve.w $xr%u, $xr%u, 0x%x", xd, xj, idx);
	}

	static int XVSETANYEQZ_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t cd = instr.whole & 0x7;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "xvsetanyeqz.b $fcc%u, $xr%u", cd, xj);
	}

	static int XVSEQ_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvseq.b $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVSETEQZ_V(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t cd = instr.whole & 0x7;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		return snprintf(buf, len, "xvseteqz.v $fcc%u, $xr%u", cd, xj);
	}

	static int XVPERMI_Q(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t imm = (instr.whole >> 10) & 0xFF;
		return snprintf(buf, len, "xvpermi.q $xr%u, $xr%u, 0x%x", xd, xj, imm);
	}

	static int XVLDX(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) LA_COLD_PATH() {
		return snprintf(buf, len, "xvldx $xr%u, %s, %s",
			instr.r3.rd, reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int XVSTX(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) LA_COLD_PATH() {
		return snprintf(buf, len, "xvstx $xr%u, %s, %s",
			instr.r3.rd, reg_name(instr.r3.rj), reg_name(instr.r3.rk));
	}

	static int XVFADD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvfadd.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVFMUL_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvfmul.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVFDIV_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvfdiv.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVFSUB_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvfsub.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVBITREVI_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t imm = (instr.whole >> 10) & 0x3F;
		return snprintf(buf, len, "xvbitrevi.d $xr%u, $xr%u, 0x%x", xd, xj, imm);
	}

	static int XVREPLVE_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvreplve.d $xr%u, $xr%u, $r%u", xd, xk, xj);
	}

	static int XVFMADD_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "xvfmadd.s $xr%u, $xr%u, $xr%u, $xr%u", xd, xj, xk, xa);
	}

	static int XVFMADD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "xvfmadd.d $xr%u, $xr%u, $xr%u, $xr%u", xd, xj, xk, xa);
	}

	static int XVFMSUB_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "xvfmsub.s $xr%u, $xr%u, $xr%u, $xr%u", xd, xj, xk, xa);
	}

	static int XVFMSUB_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "xvfmsub.d $xr%u, $xr%u, $xr%u, $xr%u", xd, xj, xk, xa);
	}

	static int XVFNMADD_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "xvfnmadd.s $xr%u, $xr%u, $xr%u, $xr%u", xd, xj, xk, xa);
	}

	static int XVFNMADD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "xvfnmadd.d $xr%u, $xr%u, $xr%u, $xr%u", xd, xj, xk, xa);
	}

	static int XVFNMSUB_S(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "xvfnmsub.s $xr%u, $xr%u, $xr%u, $xr%u", xd, xj, xk, xa);
	}

	static int XVFNMSUB_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		uint32_t xa = (instr.whole >> 15) & 0x1F;
		return snprintf(buf, len, "xvfnmsub.d $xr%u, $xr%u, $xr%u, $xr%u", xd, xj, xk, xa);
	}

	static int XVORI_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t imm8 = (instr.whole >> 10) & 0xFF;
		return snprintf(buf, len, "xvori.b $xr%u, $xr%u, 0x%x", xd, xj, imm8);
	}

	static int XVXORI_B(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t imm8 = (instr.whole >> 10) & 0xFF;
		return snprintf(buf, len, "xvxori.b $xr%u, $xr%u, 0x%x", xd, xj, imm8);
	}

	static int XVILVL_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvilvl.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVILVH_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvilvh.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVPERMI_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t imm8 = (instr.whole >> 10) & 0xFF;
		return snprintf(buf, len, "xvpermi.d $xr%u, $xr%u, 0x%x", xd, xj, imm8);
	}

	static int XVPACKEV_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvpackev.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVPACKOD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvpackod.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVPICKEV_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvpickev.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVPICKEV_W(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvpickev.w $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVPICKOD_D(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t xk = (instr.whole >> 10) & 0x1F;
		return snprintf(buf, len, "xvpickod.d $xr%u, $xr%u, $xr%u", xd, xj, xk);
	}

	static int XVLDI(char* buf, size_t len, const cpu_t&, la_instruction instr, addr_t) {
		uint32_t xd = instr.whole & 0x1F;
		uint32_t imm13 = (instr.whole >> 5) & 0x1FFF;
		return snprintf(buf, len, "xvldi $xr%u, %d", xd, (int)(int16_t)(imm13 << 3) >> 3);
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
		if (instr.r2.rd == 0) return; // Writes to x0 are discarded
		uint32_t xj = (instr.whole >> 5) & 0x1F;
		uint32_t ui3 = (instr.whole >> 10) & 0x7; // 3-bit index for 8 words

		const auto& src = cpu.registers().getvr(xj);
		cpu.reg(instr.r2.rd) = static_cast<int64_t>(static_cast<int32_t>(src.wu[ui3]));
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
						dst.du[i] = (val1 < val2) ? UINT64_MAX : 0;
					}
					break;
				case 0x04: // CEQ - Equal (ordered)
				case 0x05: // SEQ - Signaling Equal (ordered)
					if (std::isnan(val1) || std::isnan(val2)) {
						dst.du[i] = 0;
					} else {
						dst.du[i] = (val1 == val2) ? UINT64_MAX : 0;
					}
					break;
				case 0x06: // CLE - (Quiet) Less or Equal (ordered)
				case 0x07: // SLE - Signaling Less or Equal (ordered)
					if (std::isnan(val1) || std::isnan(val2)) {
						dst.du[i] = 0;
					} else {
						dst.du[i] = (val1 <= val2) ? UINT64_MAX : 0;
					}
					break;
				case 0x0E: // CULE - (Quiet) Unordered or Less or Equal
				case 0x0F: // SULE - Signaling Unordered or Less or Equal
					if (std::isnan(val1) || std::isnan(val2)) {
						dst.du[i] = UINT64_MAX;
					} else {
						dst.du[i] = (val1 <= val2) ? UINT64_MAX : 0;
					}
					break;
				case 0x14: // COR - (Quiet) Ordered
					if (std::isnan(val1) || std::isnan(val2)) {
						dst.du[i] = 0;
					} else {
						dst.du[i] = UINT64_MAX;
					}
					break;
				case 0x18: // CUNE - (Quiet) Unordered or Not Equal
				case 0x19: // SUNE - Signaling Unordered or Not Equal
					if (std::isnan(val1) || std::isnan(val2)) {
						dst.du[i] = UINT64_MAX;
					} else {
						dst.du[i] = (val1 != val2) ? UINT64_MAX : 0;
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
		__int128 q0 = (__int128)src1.d[0] + (__int128)src1.d[1];
		__int128 q1 = (__int128)src1.d[2] + (__int128)src1.d[3];

		// Store quadwords as pairs of doublewords
		auto& dst = cpu.registers().getvr(xd);
		dst.d[0] = (int64_t)q0;
		dst.d[1] = (int64_t)(q0 >> 64);
		dst.d[2] = (int64_t)q1;
		dst.d[3] = (int64_t)(q1 >> 64);
	}

}; // InstrPrintersLASX

} // loongarch
