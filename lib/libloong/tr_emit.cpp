#include "machine.hpp"
#include "decoder_cache.hpp"
#include "la_instr.hpp"
#include "tr_types.hpp"
#include <inttypes.h>
#include <sstream>

namespace loongarch {

// Helper function to format addresses as hex strings
static std::string hex_address(uint64_t addr) {
	char buf[64];
	snprintf(buf, sizeof(buf), "0x%" PRIx64, addr);
	return std::string(buf);
}

// Minimal instruction emitter for LoongArch binary translation
// This is a simplified version focusing on the most common instructions
struct Emitter
{
	std::string code;
	const TransInfo& tinfo;
	address_t current_pc;
	std::string func_name;
	unsigned m_instr_counter = 0; // Counter for instructions not yet emitted

	Emitter(const TransInfo& info)
	  : tinfo(info), current_pc(info.basepc)
	{
		char buf[64];
		snprintf(buf, sizeof(buf), "f_%lx", (unsigned long)info.basepc);
		func_name = buf;
	}

	void add_code(const std::string& line) {
		code += line + "\n";
	}

	// Flush the instruction counter to generated code
	void flush_instruction_counter() {
		auto icount = this->m_instr_counter;
		this->m_instr_counter = 0;
		if (icount > 0 && !tinfo.ignore_instruction_limit)
			code += "ic += " + std::to_string(icount) + ";\n";
	}

	// Increment internal counter (not emitted until flush)
	void increment_counter() {
		m_instr_counter++;
	}

	// Generate register access string
	std::string reg(unsigned idx) {
		if (idx == 0) return "0ULL";
		return "cpu->r[" + std::to_string(idx) + "]";
	}

	// Generate FP register access
	std::string freg(unsigned idx) {
		return "cpu->fr[" + std::to_string(idx) + "]";
	}

	// Emit a simple arithmetic/logic instruction
	void emit_alu_rri(const char* op, unsigned rd, unsigned rj, int64_t imm) {
		if (rd == 0) return; // Writing to zero register is NOP
		add_code("  " + reg(rd) + " = " + reg(rj) + " " + op + " " + std::to_string(imm) + "ULL;");
	}

	void emit_alu_rrr(const char* op, unsigned rd, unsigned rj, unsigned rk) {
		if (rd == 0) return;
		add_code("  " + reg(rd) + " = " + reg(rj) + " " + op + " " + reg(rk) + ";");
	}

	// Emit memory load
	void emit_load(const std::string& type, unsigned rd, unsigned rj, int64_t offset) {
		if (rd == 0) return;
		std::string addr = reg(rj) + " + " + std::to_string(offset) + "ULL";
		add_code("  " + reg(rd) + " = (" + type + ")rd64(cpu, " + addr + ");");
	}

	// Emit memory store
	void emit_store(const std::string& type, unsigned rd, unsigned rj, int64_t offset) {
		std::string addr = reg(rj) + " + " + std::to_string(offset) + "ULL";
		add_code("  wr64(cpu, " + addr + ", (" + type + ")" + reg(rd) + ");");
	}

	// Fallback to slow-path handler
	void emit_fallback(uint32_t instr_bits) {
		// For unimplemented instructions, call the slow-path handler
		Instruction instr = CPU::decode(la_instruction(instr_bits));
		const uintptr_t handler_func = reinterpret_cast<uintptr_t>(instr.handler);
		add_code("  ((handler_t)" + hex_address(handler_func) + ")(cpu, " + hex_address(instr_bits) + ");");
	}

	// Emit a conditional branch (1-register format: BEQZ, BNEZ)
	void emit_branch_1r(const std::string& cond, unsigned rj, address_t target) {
		increment_counter();
		flush_instruction_counter();

		std::string cond_str = reg(rj) + " " + cond;
		add_code("if (" + cond_str + ")");

		// Check if target is within current block
		if (target >= tinfo.basepc && target < tinfo.endpc) {
			// Local jump within block
			char label[64];
			snprintf(label, sizeof(label), "label_%lx", (unsigned long)target);
			add_code("  goto " + std::string(label) + ";");
		} else {
			// External jump - set PC and return
			add_code("{  cpu->pc = " + hex_address(target) + "ULL;");
			if (!tinfo.ignore_instruction_limit)
				add_code("  return (ReturnValues){ic, max_ic};");
			else
				add_code("  return (ReturnValues){0, max_ic};");
			add_code("}");
		}
	}

	// Emit a conditional branch (2-register format: BEQ, BNE, BLT, BGE, BLTU, BGEU)
	void emit_branch_2r(const std::string& cond, bool is_signed, unsigned rd, unsigned rj, address_t target) {
		increment_counter();
		flush_instruction_counter();

		std::string cond_str;
		if (is_signed) {
			cond_str = "(int64_t)" + reg(rd) + " " + cond + " (int64_t)" + reg(rj);
		} else {
			cond_str = reg(rd) + " " + cond + " " + reg(rj);
		}

		add_code("if (" + cond_str + ") {");

		// Check if target is within current block
		if (target >= tinfo.basepc && target < tinfo.endpc) {
			// Local jump within block
			char label[64];
			snprintf(label, sizeof(label), "label_%lx", (unsigned long)target);
			add_code("  goto " + std::string(label) + ";");
		} else {
			// External jump - set PC and return
			add_code("  cpu->pc = " + hex_address(target) + "ULL;");
			if (!tinfo.ignore_instruction_limit)
				add_code("  return (ReturnValues){ic, max_ic};");
			else
				add_code("  return (ReturnValues){0, max_ic};");
		}
		add_code("}");
	}

	// Emit unconditional jump (B)
	void emit_jump(address_t target) {
		increment_counter();
		flush_instruction_counter();

		// Check if target is within current block
		if (target >= tinfo.basepc && target < tinfo.endpc) {
			// Local jump within block
			char label[64];
			snprintf(label, sizeof(label), "label_%lx", (unsigned long)target);
			add_code("goto " + std::string(label) + ";");
		} else {
			// External jump - set PC and return
			add_code("cpu->pc = " + hex_address(target) + "ULL;");
			if (!tinfo.ignore_instruction_limit)
				add_code("return (ReturnValues){ic, max_ic};");
			else
				add_code("return (ReturnValues){0, max_ic};");
		}
	}

	// Emit call (BL - branch and link)
	void emit_call(unsigned rd, address_t target, address_t return_addr) {
		increment_counter();
		flush_instruction_counter();

		// Store return address
		if (rd != 0) {
			add_code(reg(rd) + " = " + hex_address(return_addr) + "ULL;");
		}

		// Jump to target
		add_code("cpu->pc = " + hex_address(target) + "ULL;");
		if (!tinfo.ignore_instruction_limit)
			add_code("return (ReturnValues){ic, max_ic};");
		else
			add_code("return (ReturnValues){0, max_ic};");
	}

	// Emit indirect jump (JIRL)
	void emit_jirl(unsigned rd, unsigned rj, int64_t offset, address_t return_addr) {
		increment_counter();
		flush_instruction_counter();

		// Store return address
		if (rd != 0) {
			add_code(reg(rd) + " = " + hex_address(return_addr) + "ULL;");
		}

		// Indirect jump
		if (offset == 0) {
			add_code("cpu->pc = " + reg(rj) + ";");
		} else {
			add_code("cpu->pc = " + reg(rj) + " + " + std::to_string(offset) + "LL;");
		}

		if (!tinfo.ignore_instruction_limit)
			add_code("return (ReturnValues){ic, max_ic};");
		else
			add_code("return (ReturnValues){0, max_ic};");
	}

	address_t pc() const { return current_pc; }
	void advance_pc() { current_pc += 4; }

	// Emit instruction trace (if enabled)
	void emit_trace(uint32_t instr_bits) {
		if (!tinfo.trace_instructions)
			return;

		// The trace callback will be responsible for decoding and printing
		// We just pass the PC and instruction bits
		char buf[256];
		snprintf(buf, sizeof(buf),
			"  api.trace(cpu, \"%s\", 0x%lx, 0x%08x);",
			func_name.c_str(), (unsigned long)current_pc, instr_bits);
		add_code(buf);
	}
};

// Generate C code for a block of instructions and return mappings
std::vector<TransMapping<>> emit(std::string& code, const TransInfo& tinfo)
{
	Emitter emit(tinfo);

	// Generate function prologue
	emit.add_code("static ReturnValues " + emit.func_name + "(CPU* cpu, uint64_t ic, uint64_t max_ic, addr_t pc) {");

	// Jump table for local jumps within the block
	if (!tinfo.jump_locations.empty()) {
		emit.add_code("jump_table:");
		emit.add_code("  switch (pc) {");
		for (address_t jump_target : tinfo.jump_locations) {
			char label[64];
			snprintf(label, sizeof(label), "  case 0x%lx: goto label_%lx;",
				(unsigned long)jump_target, (unsigned long)jump_target);
			emit.add_code(label);
		}
		emit.add_code("  default: break;");
		emit.add_code("  }");
		emit.add_code("");
	}

	// Process each instruction
	for (size_t i = 0; i < tinfo.instr.size(); i++) {
		uint32_t instr_bits = tinfo.instr[i];
		la_instruction instr{instr_bits};

		// Add label if this is a jump target
		if (tinfo.jump_locations.count(emit.pc())) {
			char label[64];
			snprintf(label, sizeof(label), "label_%lx:", (unsigned long)emit.pc());
			emit.add_code(label);
			// Flush instruction counter at jump targets
			if (emit.pc() != tinfo.basepc)
				emit.flush_instruction_counter();
		}

		// Emit trace call if tracing is enabled
		emit.emit_trace(instr_bits);

		// Decode and emit based on opcode
		const uint32_t opcode = (instr_bits >> 26) & 0x3F;
		bool handled = false;

		// BEQZ (opcode 0x10): 1RI21 format
		if (opcode == 0x10) {
			int64_t offs = InstructionHelpers::sign_extend_21(instr.ri21.offs_lo, instr.ri21.offs_hi);
			address_t target = emit.pc() + (offs << 2);
			emit.emit_branch_1r("== 0", instr.ri21.rj, target);
			handled = true;
		}
		// BNEZ (opcode 0x11): 1RI21 format
		else if (opcode == 0x11) {
			int64_t offs = InstructionHelpers::sign_extend_21(instr.ri21.offs_lo, instr.ri21.offs_hi);
			address_t target = emit.pc() + (offs << 2);
			emit.emit_branch_1r("!= 0", instr.ri21.rj, target);
			handled = true;
		}
		// B (opcode 0x14): I26 format - unconditional jump
		else if (opcode == 0x14) {
			int64_t offs = InstructionHelpers::sign_extend_26(instr.i26.offs());
			address_t target = emit.pc() + (offs << 2);
			emit.emit_jump(target);
			handled = true;
		}
		// BL (opcode 0x15): I26 format - call
		else if (opcode == 0x15) {
			int64_t offs = InstructionHelpers::sign_extend_26(instr.i26.offs());
			address_t target = emit.pc() + (offs << 2);
			address_t return_addr = emit.pc() + 4;
			emit.emit_call(1, target, return_addr); // rd=1 (ra) for BL
			handled = true;
		}
		// BEQ (opcode 0x16): 2RI16 format
		else if (opcode == 0x16) {
			int64_t offs = InstructionHelpers::sign_extend_16(instr.ri16.imm);
			address_t target = emit.pc() + (offs << 2);
			emit.emit_branch_2r("==", false, instr.ri16.rd, instr.ri16.rj, target);
			handled = true;
		}
		// BNE (opcode 0x17): 2RI16 format
		else if (opcode == 0x17) {
			int64_t offs = InstructionHelpers::sign_extend_16(instr.ri16.imm);
			address_t target = emit.pc() + (offs << 2);
			emit.emit_branch_2r("!=", false, instr.ri16.rd, instr.ri16.rj, target);
			handled = true;
		}
		// BLT (opcode 0x18): 2RI16 format
		else if (opcode == 0x18) {
			int64_t offs = InstructionHelpers::sign_extend_16(instr.ri16.imm);
			address_t target = emit.pc() + (offs << 2);
			emit.emit_branch_2r("<", true, instr.ri16.rd, instr.ri16.rj, target);
			handled = true;
		}
		// BGE (opcode 0x19): 2RI16 format
		else if (opcode == 0x19) {
			int64_t offs = InstructionHelpers::sign_extend_16(instr.ri16.imm);
			address_t target = emit.pc() + (offs << 2);
			emit.emit_branch_2r(">=", true, instr.ri16.rd, instr.ri16.rj, target);
			handled = true;
		}
		// BLTU (opcode 0x1a): 2RI16 format
		else if (opcode == 0x1a) {
			int64_t offs = InstructionHelpers::sign_extend_16(instr.ri16.imm);
			const address_t target = emit.pc() + (offs << 2);
			emit.emit_branch_2r("<", false, instr.ri16.rd, instr.ri16.rj, target);
			handled = true;
		}
		// BGEU (opcode 0x1b): 2RI16 format
		else if (opcode == 0x1b) {
			int64_t offs = InstructionHelpers::sign_extend_16(instr.ri16.imm);
			const address_t target = emit.pc() + (offs << 2);
			emit.emit_branch_2r(">=", false, instr.ri16.rd, instr.ri16.rj, target);
			handled = true;
		}
		// JIRL (opcode 0x13): 2RI16 format - indirect jump
		else if (opcode == 0x13) {
			int64_t offs = InstructionHelpers::sign_extend_16(instr.ri16.imm);
			const address_t return_addr = emit.pc() + 4;
			emit.emit_jirl(instr.ri16.rd, instr.ri16.rj, offs << 2, return_addr);
			handled = true;
		}
		// PCADDI (opcode 0x18): rd = PC + sign_ext(imm20 << 2)
		else if ((instr_bits & 0xFE000000) == 0x18000000) {
			int64_t si20 = InstructionHelpers::sign_extend_20(instr.ri20.imm);
			const int64_t offset = si20 << 2;
			if (instr.ri20.rd != 0) {
				emit.add_code("  " + emit.reg(instr.ri20.rd) + " = " +
					hex_address(emit.pc() + offset) + "ULL;");
			}
			emit.increment_counter();
			handled = true;
		}
		// PCADDU12I (opcode 0x1C): rd = PC + sign_ext(imm20 << 12)
		else if ((instr_bits & 0xFE000000) == 0x1C000000) {
			int64_t si20 = InstructionHelpers::sign_extend_20(instr.ri20.imm);
			const int64_t offset = si20 << 12;
			if (instr.ri20.rd != 0) {
				emit.add_code("  " + emit.reg(instr.ri20.rd) + " = " +
					hex_address(emit.pc() + offset) + "ULL;");
			}
			emit.increment_counter();
			handled = true;
		}
		// PCALAU12I (opcode 0x1A): rd = (PC & ~0xFFF) + sign_ext(imm20 << 12)
		else if ((instr_bits & 0xFE000000) == 0x1A000000) {
			address_t pc_aligned = emit.pc() & ~((address_t)0xFFF);
			int64_t offset = (int64_t)(int32_t)(instr.ri20.imm << 12);
			if (instr.ri20.rd != 0) {
				emit.add_code("  " + emit.reg(instr.ri20.rd) + " = " +
					hex_address(pc_aligned + offset) + "ULL;");
			}
			emit.increment_counter();
			handled = true;
		}
		// PCADDU18I (opcode 0x1E): rd = PC + sign_ext(imm20 << 18)
		else if ((instr_bits & 0xFE000000) == 0x1E000000) {
			int64_t si20 = InstructionHelpers::sign_extend_20(instr.ri20.imm);
			const int64_t offset = si20 << 18;
			if (instr.ri20.rd != 0) {
				emit.add_code("  " + emit.reg(instr.ri20.rd) + " = " +
					hex_address(emit.pc() + offset) + "ULL;");
			}
			emit.increment_counter();
			handled = true;
		}
		// SYSCALL 0
		else if (instr_bits == Opcode::SYSCALL) {
			// Emit syscall handling code
			emit.add_code("cpu->pc = pc;");
			emit.add_code("if (do_syscall(cpu, ic, max_ic, " + emit.reg(REG_A7) + ")) {");
			emit.add_code("  cpu->pc += 4; return (ReturnValues){ic, MAX_COUNTER(cpu)}; }");
			if (!tinfo.ignore_instruction_limit)
				emit.add_code("max_ic = MAX_COUNTER(cpu);");
			handled = true;
		}
		// LU12I.W (opcode 0x14): Load upper 12 bits immediate word
		else if ((instr_bits & 0xFE000000) == 0x14000000) {
			// rd = SignExtend({si20, 12'b0}, GRLEN)
			if (instr.ri20.rd != 0) {
				const int32_t result = (int32_t)(instr.ri20.imm << 12);
				emit.add_code("  " + emit.reg(instr.ri20.rd) + " = " +
					std::to_string((int64_t)result) + "LL;");
			}
			emit.increment_counter();
			handled = true;
		}
		// ADDI.D (opcode 0x2c): Add immediate doubleword
		else if ((instr_bits & 0xFFC00000) == 0x02C00000) {
			int32_t si12 = InstructionHelpers::sign_extend_12(instr.ri12.imm);
			if (instr.ri12.rd != 0) {
				emit.add_code("  " + emit.reg(instr.ri12.rd) + " = " +
					emit.reg(instr.ri12.rj) + " + " + std::to_string((int64_t)si12) + "LL;");
			}
			emit.increment_counter();
			handled = true;
		}
		// ADDI.W (opcode 0x28): Add immediate word
		else if ((instr_bits & 0xFFC00000) == 0x02800000) {
			int32_t si12 = InstructionHelpers::sign_extend_12(instr.ri12.imm);
			if (instr.ri12.rd != 0) {
				emit.add_code("  " + emit.reg(instr.ri12.rd) + " = (int64_t)((int32_t)" +
					emit.reg(instr.ri12.rj) + " + " + std::to_string(si12) + ");");
			}
			emit.increment_counter();
			handled = true;
		}
		// LD.D (opcode 0x28c): Load doubleword
		else if ((instr_bits & 0xFFC00000) == 0x28C00000) {
			int32_t si12 = InstructionHelpers::sign_extend_12(instr.ri12.imm);
			if (instr.ri12.rd != 0) {
				std::string addr = emit.reg(instr.ri12.rj) + " + " + std::to_string((int64_t)si12) + "LL";
				emit.add_code("  " + emit.reg(instr.ri12.rd) + " = rd64(cpu, " + addr + ");");
			}
			emit.increment_counter();
			handled = true;
		}
		// ST.D (opcode 0x29c): Store doubleword
		else if ((instr_bits & 0xFFC00000) == 0x29C00000) {
			int32_t si12 = InstructionHelpers::sign_extend_12(instr.ri12.imm);
			std::string addr = emit.reg(instr.ri12.rj) + " + " + std::to_string((int64_t)si12) + "LL";
			emit.add_code("  wr64(cpu, " + addr + ", " + emit.reg(instr.ri12.rd) + ");");
			emit.increment_counter();
			handled = true;
		}
		// ADD.D (opcode 0x00108): Add doubleword registers
		else if ((instr_bits & 0xFFFF8000) == 0x00108000) {
			if (instr.r3.rd != 0) {
				emit.add_code("  " + emit.reg(instr.r3.rd) + " = " +
					emit.reg(instr.r3.rj) + " + " + emit.reg(instr.r3.rk) + ";");
			}
			emit.increment_counter();
			handled = true;
		}
		// OR (opcode 0x00150): Logical OR - also handles MOVE pseudo-instruction
		else if ((instr_bits & 0xFFFF8000) == 0x00150000) {
			if (instr.r3.rd != 0) {
				// Check if this is a MOVE (OR rd, rj, zero)
				if (instr.r3.rk == 0) {
					emit.add_code("  " + emit.reg(instr.r3.rd) + " = " + emit.reg(instr.r3.rj) + ";");
				} else if (instr.r3.rj == 0) {
					emit.add_code("  " + emit.reg(instr.r3.rd) + " = " + emit.reg(instr.r3.rk) + ";");
				} else {
					emit.add_code("  " + emit.reg(instr.r3.rd) + " = " +
						emit.reg(instr.r3.rj) + " | " + emit.reg(instr.r3.rk) + ";");
				}
			}
			emit.increment_counter();
			handled = true;
		}
		// ORI (opcode 0x38): Logical OR immediate - also handles LI.W pseudo-instruction
		else if ((instr_bits & 0xFFC00000) == 0x03800000) {
			if (instr.ri12.rd != 0) {
				uint32_t imm12 = instr.ri12.imm;
				emit.add_code("  " + emit.reg(instr.ri12.rd) + " = " +
					emit.reg(instr.ri12.rj) + " | " + std::to_string(imm12) + "ULL;");
			}
			emit.increment_counter();
			handled = true;
		}

		if (!handled) {
			// For unimplemented instructions, use fallback
			emit.emit_fallback(instr_bits);
			// Still count the instruction
			emit.increment_counter();
		}

		emit.advance_pc();
	}

	emit.add_code("  cpu->pc = pc;");
	emit.add_code("  return (ReturnValues){ic, max_ic};");
	emit.add_code("}");

	// Append the generated code
	code += emit.code;

	// Create a mapping for the block entry point
	std::vector<TransMapping<>> mappings;
	mappings.push_back({tinfo.basepc, emit.func_name, 0});

	return mappings;
}

} // loongarch
