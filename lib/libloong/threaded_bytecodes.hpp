#pragma once

namespace loongarch
{
	// Bytecodes for threaded dispatch
	// Following libriscv model: specific bytecodes for popular instructions
	enum
	{
		LA64_BC_INVALID = 0,       // Invalid instruction

		// Popular instructions (top 5 from profiling)
		LA64_BC_LD_D,              // Load doubleword (10355 occurrences)
		LA64_BC_OR,                // Bitwise OR (9153 occurrences)
		LA64_BC_ST_D,              // Store doubleword (7976 occurrences)
		LA64_BC_ADDI_W,            // Add immediate word (6030 occurrences)
		LA64_BC_ADDI_D,            // Add immediate doubleword (5195 occurrences)
		LA64_BC_ANDI,              // AND immediate (2702 occurrences in stream.elf)
		LA64_BC_ADD_D,             // Add doubleword (1867 occurrences in stream.elf)
		LA64_BC_SUB_D,             // Subtract doubleword (1116 occurrences in stream.elf)
		LA64_BC_ORI,               // OR immediate (948 occurrences in stream.elf)
		LA64_BC_SLLI_W,            // Shift left logical immediate word (1021 occurrences in stream.elf)
		LA64_BC_SLLI_D,            // Shift left logical immediate doubleword (699 in coremark, 687 in stream)
		LA64_BC_LD_BU,             // Load byte unsigned (838 in coremark, 807 in stream)
		LA64_BC_ST_B,              // Store byte (870 in coremark, 857 in stream)
		LA64_BC_ST_W,              // Store word (624 in stream)
		LA64_BC_PCADDI,            // PC-relative add immediate (2586 in stream)
		LA64_BC_PCALAU12I,         // PC-aligned add upper immediate (621 in stream)
		LA64_BC_LDPTR_D,           // Load pointer doubleword (2183 in stream)
		LA64_BC_LDPTR_W,           // Load pointer word (1710 in stream)
		LA64_BC_STPTR_D,           // Store pointer doubleword (1114 in stream)
		LA64_BC_LU12I_W,           // Load upper 12-bit immediate word (877 in stream)
		LA64_BC_BSTRPICK_D,        // Bit string pick doubleword (615 in stream)
		LA64_BC_AND,               // Bitwise AND (565 in stream)
		LA64_BC_ALSL_D,            // Arithmetic left shift and add doubleword (365 in stream)
		LA64_BC_SRLI_D,            // Shift right logical immediate doubleword (271 in stream)
		LA64_BC_LD_B,              // Load byte signed (495 in stream)
		LA64_BC_STPTR_W,           // Store pointer word (270 in stream)
		LA64_BC_LDX_D,             // Load doubleword indexed (369 in stream)
		LA64_BC_MASKEQZ,           // Mask if equal to zero (256 in stream)
		LA64_BC_MASKNEZ,           // Mask if not equal to zero (257 in stream)
		LA64_BC_MUL_D,             // Multiply doubleword (224 in stream)
		LA64_BC_SUB_W,             // Subtract word (210 in stream)
		LA64_BC_SLL_D,             // Shift left logical doubleword (184 in stream)
		LA64_BC_STX_D,             // Store doubleword indexed (178 in stream)
		LA64_BC_BSTRPICK_W,        // Bit string pick word (167 in stream)
		LA64_BC_SLTU,              // Set if less than unsigned (167 in stream)
		LA64_BC_LDX_W,             // Load word indexed (163 in stream)
		LA64_BC_STX_W,             // Store word indexed (157 in stream)
		LA64_BC_XOR,               // Bitwise XOR (154 in stream)
		LA64_BC_LD_HU,             // Load halfword unsigned (244 in stream)
		LA64_BC_ADD_W,             // Add word (131 in stream)
		LA64_BC_SRAI_D,            // Shift right arithmetic immediate doubleword (121 in stream)
		LA64_BC_EXT_W_B,           // Extend byte to word with sign (125 in stream)
		LA64_BC_LDX_BU,            // Load byte unsigned indexed (132 in stream)
		LA64_BC_BSTRINS_D,         // Bit string insert doubleword (115 in stream)
		LA64_BC_LU32I_D,           // Load upper 32-bit immediate doubleword (93 in stream)

		// Branch instructions
		LA64_BC_BEQZ,              // Branch if equal to zero
		LA64_BC_BNEZ,              // Branch if not equal to zero
		LA64_BC_BEQ,               // Branch if equal
		LA64_BC_BNE,               // Branch if not equal
		LA64_BC_JIRL,              // Jump indirect and link register (1513 in stream)
		LA64_BC_B,                 // Unconditional branch
		LA64_BC_BL,                // Branch and link
		LA64_BC_BLT,               // Branch if less than
		LA64_BC_BGE,               // Branch if greater than or equal
		LA64_BC_BLTU,              // Branch if less than unsigned
		LA64_BC_BGEU,              // Branch if greater than or equal unsigned

		// Generic handlers
		LA64_BC_FUNCTION,          // Non-PC-modifying instruction (simple handler call)
		LA64_BC_FUNCBLOCK,         // PC-modifying instruction (branches, jumps, PC-relative)
		LA64_BC_SYSCALL,           // System call (needs special handling)
		LA64_BC_STOP,              // Stop execution marker
		BYTECODES_MAX
	};
	static_assert(BYTECODES_MAX <= 256, "A bytecode must fit in a byte");

	// Get the name of a bytecode
	static inline const char* bytecode_name(uint8_t bytecode)
	{
		switch (bytecode) {
		case LA64_BC_INVALID: return "INVALID";
		case LA64_BC_LD_D: return "LD.D";
		case LA64_BC_OR: return "OR";
		case LA64_BC_ST_D: return "ST.D";
		case LA64_BC_ADDI_W: return "ADDI.W";
		case LA64_BC_ADDI_D: return "ADDI.D";
		case LA64_BC_ANDI: return "ANDI";
		case LA64_BC_ADD_D: return "ADD.D";
		case LA64_BC_SUB_D: return "SUB.D";
		case LA64_BC_ORI: return "ORI";
		case LA64_BC_SLLI_W: return "SLLI.W";
		case LA64_BC_SLLI_D: return "SLLI.D";
		case LA64_BC_LD_BU: return "LD.BU";
		case LA64_BC_ST_B: return "ST.B";
		case LA64_BC_ST_W: return "ST.W";
		case LA64_BC_PCADDI: return "PCADDI";
		case LA64_BC_PCALAU12I: return "PCALAU12I";
		case LA64_BC_LDPTR_D: return "LDPTR.D";
		case LA64_BC_LDPTR_W: return "LDPTR.W";
		case LA64_BC_STPTR_D: return "STPTR.D";
		case LA64_BC_LU12I_W: return "LU12I.W";
		case LA64_BC_BSTRPICK_D: return "BSTRPICK.D";
		case LA64_BC_AND: return "AND";
		case LA64_BC_ALSL_D: return "ALSL.D";
		case LA64_BC_SRLI_D: return "SRLI.D";
		case LA64_BC_LD_B: return "LD.B";
		case LA64_BC_STPTR_W: return "STPTR.W";
		case LA64_BC_LDX_D: return "LDX.D";
		case LA64_BC_MASKEQZ: return "MASKEQZ";
		case LA64_BC_MASKNEZ: return "MASKNEZ";
		case LA64_BC_MUL_D: return "MUL.D";
		case LA64_BC_SUB_W: return "SUB.W";
		case LA64_BC_SLL_D: return "SLL.D";
		case LA64_BC_STX_D: return "STX.D";
		case LA64_BC_BSTRPICK_W: return "BSTRPICK.W";
		case LA64_BC_SLTU: return "SLTU";
		case LA64_BC_LDX_W: return "LDX.W";
		case LA64_BC_STX_W: return "STX.W";
		case LA64_BC_XOR: return "XOR";
		case LA64_BC_LD_HU: return "LD.HU";
		case LA64_BC_ADD_W: return "ADD.W";
		case LA64_BC_SRAI_D: return "SRAI.D";
		case LA64_BC_EXT_W_B: return "EXT.W.B";
		case LA64_BC_LDX_BU: return "LDX.BU";
		case LA64_BC_BSTRINS_D: return "BSTRINS.D";
		case LA64_BC_LU32I_D: return "LU32I.D";
		case LA64_BC_BEQZ: return "BEQZ";
		case LA64_BC_BNEZ: return "BNEZ";
		case LA64_BC_BEQ: return "BEQ";
		case LA64_BC_BNE: return "BNE";
		case LA64_BC_JIRL: return "JIRL";
		case LA64_BC_B: return "B";
		case LA64_BC_BL: return "BL";
		case LA64_BC_BLT: return "BLT";
		case LA64_BC_BGE: return "BGE";
		case LA64_BC_BLTU: return "BLTU";
		case LA64_BC_BGEU: return "BGEU";
		case LA64_BC_FUNCTION: return "FUNCTION";
		case LA64_BC_FUNCBLOCK: return "FUNCBLOCK";
		case LA64_BC_SYSCALL: return "SYSCALL";
		case LA64_BC_STOP: return "STOP";
		default: return "UNKNOWN";
		}
	}

	// Optimized instruction formats for fast field access
	union FasterLA64_RI12 {
		uint32_t whole;
		struct {
			uint8_t rd;     // bits [4:0]
			uint8_t rj;     // bits [9:5]
			int16_t imm;    // bits [21:10] sign-extended
		};
		void set_imm(uint16_t imm12) {
			// Sign-extend 12-bit immediate to int16_t
			this->imm = int16_t(imm12 << 4) >> 4;
		}
	};

	union FasterLA64_R3 {
		uint32_t whole;
		struct {
			uint8_t rd;     // bits [4:0]
			uint8_t rj;     // bits [9:5]
			uint8_t rk;     // bits [14:10]
		};
	};

	union FasterLA64_Shift {
		uint32_t whole;
		struct {
			uint8_t rd;     // bits [4:0]
			uint8_t rj;     // bits [9:5]
			uint8_t ui5;    // bits [14:10] for shift amount (SLLI.W, etc)
		};
	};

	union FasterLA64_Shift64 {
		uint32_t whole;
		struct {
			uint8_t rd;     // bits [4:0]
			uint8_t rj;     // bits [9:5]
			uint8_t ui6;    // bits [15:10] for 64-bit shift amount (SLLI.D, etc)
		};
	};

	union FasterLA64_RI20 {
		uint32_t whole;
		struct {
			uint8_t rd;      // bits [7:0] - only bits [4:0] used
			uint8_t imm_lo;  // bits [15:8] - imm[7:0]
			int16_t imm_hi;  // bits [31:16] - imm[19:8] sign-extended
		};

		// Get the full sign-extended immediate
		int32_t get_imm() const {
			// Combine imm_hi (12 bits) and imm_lo (8 bits) to form 20-bit immediate
			// imm_hi already contains sign-extended bits [19:8]
			// imm_lo contains bits [7:0]
			return (int32_t(imm_hi) << 8) | imm_lo;
		}

		void set_imm(uint32_t imm20) {
			// imm20 is 20 bits: [19:0]
			// Store bits [7:0] in imm_lo
			imm_lo = uint8_t(imm20 & 0xFF);
			// Store bits [19:8] in imm_hi, sign-extended to 16 bits
			imm_hi = int16_t(int32_t(imm20 << 12) >> 20);  // Sign-extend from bit 19
		}
	};

	union FasterLA64_RI14 {
		uint32_t whole;
		struct {
			uint8_t rd;     // bits [4:0]
			uint8_t rj;     // bits [9:5]
			int16_t imm14;  // bits [23:10] sign-extended 14-bit immediate
		};
		void set_imm(uint16_t imm) {
			// Sign-extend 14-bit immediate to int16_t
			this->imm14 = int16_t(imm << 2) >> 2;
		}
	};

	union FasterLA64_BitField {
		uint32_t whole;
		struct {
			uint8_t rd;     // bits [4:0]
			uint8_t rj;     // bits [9:5]
			uint8_t lsbd;   // bits [15:10] - low bit position
			uint8_t msbd;   // bits [21:16] - high bit position
		};
	};

	union FasterLA64_BitFieldW {
		uint32_t whole;
		struct {
			uint8_t rd;     // bits [4:0]
			uint8_t rj;     // bits [9:5]
			uint8_t lsbw;   // bits [14:10] - low bit position (5 bits)
			uint8_t msbw;   // bits [20:16] - high bit position (5 bits)
		};
	};

	union FasterLA64_R3SA2 {
		uint32_t whole;
		struct {
			uint8_t rd;     // bits [4:0]
			uint8_t rj;     // bits [9:5]
			uint8_t rk;     // bits [14:10]
			uint8_t sa2;    // bits [16:15] - shift amount (2 bits)
		};
	};

	union FasterLA64_RI16 {
		uint32_t whole;
		struct {
			uint8_t rd;      // bits [4:0]
			uint8_t rj;      // bits [9:5]
			int16_t imm16;   // bits [25:10] sign-extended 16-bit immediate
		};
		void set_imm(uint16_t imm) {
			this->imm16 = (int16_t)imm;
		}
	};

} // namespace loongarch
