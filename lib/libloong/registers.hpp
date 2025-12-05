#pragma once
#include "common.hpp"
#include <array>
#include <cstdint>
#include <cstring>

namespace loongarch
{
	// LoongArch register names
	static constexpr inline const char* la_regname(uint32_t reg) {
		constexpr const char* regnames[] = {
			"zero", "ra", "tp", "sp", "a0", "a1", "a2", "a3",
			"a4", "a5", "a6", "a7", "t0", "t1", "t2", "t3",
			"t4", "t5", "t6", "t7", "t8", "r21", "fp", "s0",
			"s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8"
		};
		return (reg < 32) ? regnames[reg] : "unknown";
	}

	// Register file for LoongArch
	struct alignas(64) Registers
	{
		using register_t = address_t;

		// General purpose registers
		auto& get(uint32_t idx) noexcept { return m_regs[idx]; }
		const auto& get(uint32_t idx) const noexcept { return m_regs[idx]; }

		// Floating-point registers are shared with the low bits of LSX vector lanes
		auto& getfl32(uint32_t idx) noexcept { return m_vr[idx].f[0]; }
		const auto& getfl32(uint32_t idx) const noexcept { return m_vr[idx].f[0]; }

		auto& getfl64(uint32_t idx) noexcept { return m_vr[idx].df[0]; }
		const auto& getfl64(uint32_t idx) const noexcept { return m_vr[idx].df[0]; }

		auto& getvr(uint32_t idx) noexcept { return m_vr[idx]; }
		const auto& getvr(uint32_t idx) const noexcept { return m_vr[idx]; }

		auto& getvr128low(uint32_t idx) noexcept { return m_vr[idx].lsx_low; }
		const auto& getvr128low(uint32_t idx) const noexcept { return m_vr[idx].lsx_low; }

		// LASX vector registers (256-bit)
		// These share the low 32- and 64-bits with floating-point registers
		// as well as the low 128-bits with LSX vector registers.
		union alignas(32) VectorReg256 {
			int8_t   b[32];
			int16_t  h[16];
			int32_t  w[8];
			int64_t  d[4];
			uint8_t  bu[32];
			uint16_t hu[16];
			uint32_t wu[8];
			uint64_t du[4];
			float    f[8];
			double   df[4];
			std::array<uint64_t, 2> lsx_low;  // Low 128 bits for LSX compatibility
		};

		// Floating-point condition flags (FCC)
		uint8_t cf(uint32_t idx) const noexcept { return (m_fcc >> idx) & 1; }
		void set_cf(uint32_t idx, uint8_t value) noexcept {
			if (value) {
				m_fcc |= (1u << idx);
			} else {
				m_fcc &= ~(1u << idx);
			}
		}

		// Floating-point control and status register (FCSR) access
		uint32_t fcsr() const noexcept { return m_fcsr.whole; }
		void set_fcsr(uint32_t value) noexcept { m_fcsr.whole = value; }		// Reset registers

		void reset() {
			this->pc = 0;
			std::memset(m_regs.data(), 0, m_regs.size() * sizeof(register_t));
			this->m_fcsr.whole = 0;
			this->m_fcc = 0;
			std::memset(m_vr.data(), 0, m_vr.size() * sizeof(VectorReg256));
		}

		// Debug output
		std::string to_string() const;

		// PC register
		address_t pc = 0;
	private:
		std::array<register_t, 32> m_regs = {};
		std::array<VectorReg256, 32> m_vr = {};
		// Floating-point control and status register
		struct {
			uint32_t whole = 0;
		} m_fcsr;
		// Floating-point condition flags (8 flags)
		uint8_t m_fcc = 0;
	};

	// Register indices
	enum LARegister : uint32_t {
		REG_ZERO = 0,
		REG_RA   = 1,  // Return address
		REG_TP   = 2,  // Thread pointer
		REG_SP   = 3,  // Stack pointer
		REG_A0   = 4,  // Argument/return value 0
		REG_A1   = 5,  // Argument/return value 1
		REG_A2   = 6,  // Argument 2
		REG_A3   = 7,  // Argument 3
		REG_A4   = 8,  // Argument 4
		REG_A5   = 9,  // Argument 5
		REG_A6   = 10, // Argument 6
		REG_A7   = 11, // Argument 7
		REG_T0   = 12, // Temporary 0
		REG_T1   = 13, // Temporary 1
		REG_T2   = 14, // Temporary 2
		REG_T3   = 15, // Temporary 3
		REG_T4   = 16, // Temporary 4
		REG_T5   = 17, // Temporary 5
		REG_T6   = 18, // Temporary 6
		REG_T7   = 19, // Temporary 7
		REG_T8   = 20, // Temporary 8
		REG_FP   = 22, // Frame pointer
		REG_S0   = 23, // Saved register 0
		REG_S1   = 24, // Saved register 1
		REG_S2   = 25, // Saved register 2
		REG_S3   = 26, // Saved register 3
		REG_S4   = 27, // Saved register 4
		REG_S5   = 28, // Saved register 5
		REG_S6   = 29, // Saved register 6
		REG_S7   = 30, // Saved register 7
		REG_S8   = 31, // Saved register 8

		REG_FA0  = 0,  // Floating-point argument/return value 0
		REG_FA1  = 1,  // Floating-point argument/return value 1
		REG_FS0  = 24, // Floating-point saved register 0
	};

} // namespace loongarch
