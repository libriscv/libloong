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
		LA64_BC_PCADDI,            // PC-relative add immediate (2586 in stream)
		LA64_BC_PCALAU12I,         // PC-aligned add upper immediate (621 in stream)
		LA64_BC_LDPTR_D,           // Load pointer doubleword (2183 in stream)
		LA64_BC_LDPTR_W,           // Load pointer word (1710 in stream)
		LA64_BC_STPTR_D,           // Store pointer doubleword (1114 in stream)
		LA64_BC_LU12I_W,           // Load upper 12-bit immediate word (877 in stream)

		// Branch instructions
		LA64_BC_B,                 // Unconditional branch
		LA64_BC_BL,                // Branch and link
		LA64_BC_BEQZ,              // Branch if equal to zero
		LA64_BC_BNEZ,              // Branch if not equal to zero
		LA64_BC_BEQ,               // Branch if equal
		LA64_BC_BNE,               // Branch if not equal
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

} // namespace loongarch
