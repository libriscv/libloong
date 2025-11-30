#pragma once
#include "common.hpp"

namespace loongarch {

	// Instruction format (32-bit LoongArch instructions)
	union la_instruction {
		uint32_t whole;

		struct {
			uint32_t opcode : 32;
		} raw;

		// 2R-type: op rd, rj
		struct {
			uint32_t rd    : 5;
			uint32_t rj    : 5;
			uint32_t opcode : 22;
		} r2;

		// 3R-type: op rd, rj, rk
		struct {
			uint32_t rd     : 5;
			uint32_t rj     : 5;
			uint32_t rk     : 5;
			uint32_t opcode : 17;
		} r3;

		// 3R-type with sa2: op rd, rj, rk, sa2 (ALSL)
		struct {
			uint32_t rd     : 5;
			uint32_t rj     : 5;
			uint32_t rk     : 5;
			uint32_t sa2    : 2;
			uint32_t opcode : 15;
		} r3sa2;

		// 4R-type: op rd, rj, rk, ra
		struct {
			uint32_t rd     : 5;
			uint32_t rj     : 5;
			uint32_t rk     : 5;
			uint32_t ra     : 5;
			uint32_t opcode : 12;
		} r4;

		// 2RI8-type: op rd, rj, imm8
		struct {
			uint32_t rd     : 5;
			uint32_t rj     : 5;
			uint32_t imm    : 8;
			uint32_t opcode : 14;
		} ri8;

		// 2RI12-type: op rd, rj, imm12
		struct {
			uint32_t rd     : 5;
			uint32_t rj     : 5;
			uint32_t imm    : 12;
			uint32_t opcode : 10;
		} ri12;

		// 2RI14-type: op rd, rj, imm14
		struct {
			uint32_t rd     : 5;
			uint32_t rj     : 5;
			uint32_t imm    : 14;
			uint32_t opcode : 8;
		} ri14;

		// 2RI16-type: op rd, rj, imm16
		struct {
			uint32_t rd     : 5;
			uint32_t rj     : 5;
			uint32_t imm    : 16;
			uint32_t opcode : 6;
		} ri16;

		// 1RI20-type: op rd, imm20
		struct {
			uint32_t rd     : 5;
			uint32_t imm    : 20;
			uint32_t opcode : 7;
		} ri20;

		// 1RI21-type: op rj, offs21 (branches like BEQZ, BNEZ)
		// offs[20:16] at bits[4:0], rj at bits[9:5], offs[15:0] at bits[25:10]
		struct {
			uint32_t offs_hi : 5;   // bits [4:0] = offs[20:16]
			uint32_t rj      : 5;   // bits [9:5]
			uint32_t offs_lo : 16;  // bits [25:10] = offs[15:0]
			uint32_t opcode  : 6;   // bits [31:26]

			// Get the combined 21-bit offset
			constexpr uint32_t offs() const {
				return (offs_hi << 16) | offs_lo;
			}
		} ri21;

		// I26-type: op offs26 (jumps B/BL)
		// offs[25:0] is split: bits[9:0] = offs[25:16], bits[25:10] = offs[15:0]
		struct {
			uint32_t offs_hi  : 10;  // bits[9:0] = offs[25:16]
			uint32_t offs_lo  : 16;  // bits[25:10] = offs[15:0]
			uint32_t opcode   : 6;   // bits[31:26]

			// Get the combined 26-bit offset
			constexpr uint32_t offs() const {
				return (offs_hi << 16) | offs_lo;
			}
		} i26;

		constexpr la_instruction() : whole(0) {}
		constexpr la_instruction(uint32_t val) : whole(val) {}

		constexpr uint32_t opcode() const noexcept { return whole; }
		constexpr uint32_t length() const noexcept { return 4; } // All LoongArch instructions are 4 bytes
	};

	using instruction_format = la_instruction;

namespace Opcode {
	// Integer operations
	constexpr uint32_t ADD_W      = 0x00100000;
	constexpr uint32_t ADD_D      = 0x00108000;
	constexpr uint32_t SUB_W      = 0x00110000;
	constexpr uint32_t SUB_D      = 0x00118000;
	constexpr uint32_t ADDI_W     = 0x02800000;
	constexpr uint32_t ADDI_D     = 0x02c00000;

	// Comparison operations
	constexpr uint32_t SLT        = 0x00120000;
	constexpr uint32_t SLTU       = 0x00128000;
	constexpr uint32_t SLTI       = 0x02000000;  // Set on Less Than Immediate (signed)
	constexpr uint32_t SLTUI      = 0x02400000;  // Set on Less Than Unsigned Immediate

	// Logical operations
	constexpr uint32_t AND        = 0x00148000;
	constexpr uint32_t OR         = 0x00150000;
	constexpr uint32_t XOR        = 0x00158000;
	constexpr uint32_t NOR        = 0x00140000;
	constexpr uint32_t MASKEQZ    = 0x00130000;
	constexpr uint32_t MASKNEZ    = 0x00138000;
	constexpr uint32_t ANDI       = 0x03400000;
	constexpr uint32_t ORI        = 0x03800000;
	constexpr uint32_t XORI       = 0x03C00000;

	// Byte manipulation
	constexpr uint32_t BYTEPICK_D = 0x000C0000;  // mask=0xFFFC0000, sa3 in bits [17:15]

	// Shift operations
	constexpr uint32_t SLL_W      = 0x00170000;
	constexpr uint32_t SRL_W      = 0x00178000;
	constexpr uint32_t SRA_W      = 0x00180000;
	constexpr uint32_t SLL_D      = 0x00188000;

	// Shift immediate (bits[31:16] identify instruction)
	constexpr uint32_t SLLI_W     = 0x00408000;
	constexpr uint32_t SLLI_D     = 0x00410000;
	constexpr uint32_t SRLI_W     = 0x00448000;
	constexpr uint32_t SRLI_D     = 0x00450000;
	constexpr uint32_t SRAI_W     = 0x00488000;
	constexpr uint32_t SRAI_D     = 0x00490000;
	constexpr uint32_t ROTRI_D    = 0x02400000;  // Rotate Right Immediate Doubleword (op10=0x009)
	constexpr uint32_t ROTRI_W    = 0x00490000;  // Rotate Right Immediate Word (op16=0x0049)
	constexpr uint32_t ROTR_W     = 0x001B0000;  // Rotate Right Word (op17)
	constexpr uint32_t ROTR_D     = 0x001B8000;  // Rotate Right Doubleword (op17)
	constexpr uint32_t SRL_D      = 0x00190000;
	constexpr uint32_t SRA_D      = 0x00198000;
	constexpr uint32_t ALSL_W     = 0x00040000;  // Address Load Shift Left Word
	constexpr uint32_t ALSL_D     = 0x002c0000;  // Address Load Shift Left Doubleword

	// Load/Store
	constexpr uint32_t LD_B       = 0x28000000;
	constexpr uint32_t LD_H       = 0x28400000;
	constexpr uint32_t LD_W       = 0x28800000;
	constexpr uint32_t LD_D       = 0x28c00000;
	constexpr uint32_t LD_BU      = 0x2a000000;
	constexpr uint32_t LD_HU      = 0x2a400000;
	constexpr uint32_t LD_WU      = 0x2a800000;
	constexpr uint32_t ST_B       = 0x29000000;
	constexpr uint32_t ST_H       = 0x29400000;
	constexpr uint32_t ST_W       = 0x29800000;
	constexpr uint32_t ST_D       = 0x29c00000;
	constexpr uint32_t LDPTR_W    = 0x24000000;
	constexpr uint32_t STPTR_W    = 0x25000000;
	constexpr uint32_t LDPTR_D    = 0x26000000;
	constexpr uint32_t STPTR_D    = 0x27000000;

	// Floating-point load/store
	constexpr uint32_t FLD_S      = 0x2B000000;
	constexpr uint32_t FST_S      = 0x2B400000;
	constexpr uint32_t FLD_D      = 0x2B800000;
	constexpr uint32_t FST_D      = 0x2BC00000;

	// Indexed load/store (bits [31:15] identify instruction)
	constexpr uint32_t STX_B      = 0x38100000;
	constexpr uint32_t STX_H      = 0x38140000;
	constexpr uint32_t STX_W      = 0x38180000;
	constexpr uint32_t STX_D      = 0x381C0000;
	constexpr uint32_t FSTX_D     = 0x383C0000;

	// Atomic operations (bits [31:17] identify base operation, bits [16:15] are memory ordering)
	// Memory ordering: 00=none, 01=acquire, 10=release, 11=acq_rel
	constexpr uint32_t AMSWAP_W   = 0x38600000;  // Base for 32-bit swap
	constexpr uint32_t AMSWAP_D   = 0x38608000;  // Base for 64-bit swap

	// Branches
	constexpr uint32_t BEQZ       = 0x40000000;
	constexpr uint32_t BNEZ       = 0x44000000;
	constexpr uint32_t BEQ        = 0x58000000;
	constexpr uint32_t BNE        = 0x5c000000;
	constexpr uint32_t BLT        = 0x60000000;
	constexpr uint32_t BGE        = 0x64000000;
	constexpr uint32_t BLTU       = 0x68000000;
	constexpr uint32_t BGEU       = 0x6c000000;

	// Jumps
	constexpr uint32_t B          = 0x50000000;
	constexpr uint32_t BL         = 0x54000000;
	constexpr uint32_t JIRL       = 0x4c000000;

	// Upper immediates
	constexpr uint32_t LU12I_W    = 0x14000000;
	constexpr uint32_t LU32I_D    = 0x16000000;
	constexpr uint32_t PCADDI     = 0x18000000;
	constexpr uint32_t PCADDU12I  = 0x1c000000;
	constexpr uint32_t PCALAU12I  = 0x1a000000;
	constexpr uint32_t PCADDU18I  = 0x1e000000;
	constexpr uint32_t LU52I_D    = 0x03000000;		// System
	constexpr uint32_t SYSCALL    = 0x002b0000;
	constexpr uint32_t BREAK      = 0x002a0000;

	// Multiply/Divide
	constexpr uint32_t MUL_W      = 0x001c0000;
	constexpr uint32_t MULH_W     = 0x001c8000;
	constexpr uint32_t MULH_WU    = 0x001d0000;
	constexpr uint32_t MUL_D      = 0x001d8000;
	constexpr uint32_t MULH_D     = 0x001e0000;
	constexpr uint32_t MULH_DU    = 0x001e8000;
	constexpr uint32_t DIV_W      = 0x00200000;
	constexpr uint32_t MOD_W      = 0x00208000;
	constexpr uint32_t DIV_WU     = 0x00210000;
	constexpr uint32_t MOD_WU     = 0x00218000;
	constexpr uint32_t DIV_D      = 0x00220000;
	constexpr uint32_t MOD_D      = 0x00228000;
	constexpr uint32_t DIV_DU     = 0x00230000;
	constexpr uint32_t MOD_DU     = 0x00238000;

	// Bit manipulation
	constexpr uint32_t BSTRINS_W  = 0x00600000;  // op10 = 0x006, check msbw/lsbw
	constexpr uint32_t BSTRINS_D  = 0x00800000;  // op8 = 0x008, check msbd/lsbd
	constexpr uint32_t BSTRPICK_D = 0x00c00000;  // op10 = 0x003, check msbd/lsbd

	// Byte reversal (2R-type, bits[31:10] identify instruction)
	constexpr uint32_t REVB_2H    = 0x00003000;  // Reverse bytes in 2 halfwords (op22=0x00000C)
	constexpr uint32_t REVB_4H    = 0x00003400;  // Reverse bytes in 4 halfwords (op22=0x00000D)
} // Opcode

template <int W>
struct InstructionHelpers {
	using address_t = address_type<W>;

	// Sign extend immediate values
	static constexpr int32_t sign_extend_12(uint32_t val) {
		return int32_t(val << 20) >> 20;
	}

	static constexpr int32_t sign_extend_14(uint32_t val) {
		return int32_t(val << 18) >> 18;
	}

	static constexpr int32_t sign_extend_16(uint32_t val) {
		return int32_t(val << 16) >> 16;
	}

	static constexpr int32_t sign_extend_20(uint32_t val) {
		return int32_t(val << 12) >> 12;
	}

	static constexpr int32_t sign_extend_21(uint32_t offs_lo, uint32_t offs_hi) {
		uint32_t val = (offs_hi << 16) | offs_lo;
		return int32_t(val << 11) >> 11;
	}

	static constexpr int32_t sign_extend_26(uint32_t offs) {
		// offs is a 26-bit signed value
		return int32_t(offs << 6) >> 6;
	}
};

} // loongarch
