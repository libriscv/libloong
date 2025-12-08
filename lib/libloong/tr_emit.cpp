#if !defined(LA_BINARY_TRANSLATION)
#define LA_BINARY_TRANSLATION
#endif
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
	unsigned m_instr_counter = 0;

	Emitter(const TransInfo& info)
	  : tinfo(info), current_pc(info.basepc)
	{
		char buf[64];
		snprintf(buf, sizeof(buf), "f_%" PRIx64, (uint64_t)info.basepc);
		func_name = buf;
	}

	void add_code(const std::string& line) {
		code += line + "\n";
	}

	// Flush the instruction counter to generated code
	void flush_instruction_counter() {
		auto icount = this->m_instr_counter;
		this->m_instr_counter = 0;
		if (icount > 0 && !tinfo.options.translate_ignore_instruction_limit)
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
	std::string freg32(unsigned idx) {
		return "cpu->vr[" + std::to_string(idx) + "].f[0]";
	}
	std::string freg64(unsigned idx) {
		return "cpu->vr[" + std::to_string(idx) + "].df[0]";
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
	void emit_fallback(const Instruction& instr, uint32_t instr_bits) {
		// For unimplemented instructions, call the slow-path handler
		const uintptr_t handler_func = reinterpret_cast<uintptr_t>(instr.handler);
		add_code("  ((handler_t)" + hex_address(handler_func) + ")(cpu, " + hex_address(instr_bits) + ");");
	}

	void emit_return() {
		if (!tinfo.options.translate_ignore_instruction_limit)
			add_code("  return (ReturnValues){ic, max_ic};");
		else
			add_code("  return (ReturnValues){0, max_ic};");
	}

	// Emit a conditional branch (1-register format: BEQZ, BNEZ)
	void emit_branch_1r(const std::string& cond, unsigned rj, address_t target) {
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
			this->emit_return();
			add_code("}");
		}
	}

	// Emit a conditional branch (2-register format: BEQ, BNE, BLT, BGE, BLTU, BGEU)
	void emit_branch_2r(const std::string& cond, bool is_signed, unsigned rd, unsigned rj, address_t target) {
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
			this->emit_return();
		}
		add_code("}");
	}

	// Emit unconditional jump (B)
	void emit_jump(address_t target) {
		flush_instruction_counter();

		// Check if target is within current block
		if (target >= tinfo.basepc && target < tinfo.endpc) {
			// Local jump within block
			char label[64];
			snprintf(label, sizeof(label), "goto label_%" PRIx64 ";", (uint64_t)target);
			add_code(label);
		} else {
			// External jump - set PC and return
			add_code("  cpu->pc = " + hex_address(target) + "ULL;");
			this->emit_return();
		}
	}

	// Emit call (BL - branch and link)
	void emit_call(unsigned rd, address_t target, address_t return_addr) {
		flush_instruction_counter();

		// Store return address
		if (rd != 0) {
			add_code(reg(rd) + " = " + hex_address(return_addr) + "ULL;");
		}

		// Jump to target
		add_code("cpu->pc = " + hex_address(target) + "ULL;");
		this->emit_return();
	}

	// Emit indirect jump (JIRL)
	void emit_jirl(unsigned rd, unsigned rj, int64_t offset, address_t return_addr) {
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

		this->emit_return();
	}

	address_t pc() const { return current_pc; }
	void advance_pc() { current_pc += 4; }

	// Emit instruction trace (if enabled)
	void emit_trace(uint32_t instr_bits) {
		if (!tinfo.options.translate_trace)
			return;

		// The trace callback will be responsible for decoding and printing
		// We just pass the PC and instruction bits
		char buf[256];
		snprintf(buf, sizeof(buf),
			"  api.trace(cpu, \"%s\", 0x%lx, 0x%08x);",
			func_name.c_str(), (unsigned long)current_pc, instr_bits);
		add_code(buf);
	}
	void emit_custom_trace(const std::string& value) {
		if (!tinfo.options.translate_trace)
			return;

		char buf[256];
		snprintf(buf, sizeof(buf),
			"  api.trace(cpu, \"%s\", 0x%lx, %s);",
			func_name.c_str(), (unsigned long)current_pc, value.c_str());
		add_code(buf);
	}
};

// Generate C code for a block of instructions and return mappings
std::vector<TransMapping<>> emit(std::string& code, const TransInfo& tinfo)
{
	Emitter emit(tinfo);

	// Create a mapping for the block entry point
	std::vector<TransMapping<>> mappings;

	// Generate function prologue
	emit.add_code("static ReturnValues " + emit.func_name + "(CPU* cpu, uint64_t ic, uint64_t max_ic, addr_t pc) {");

	// Jump table for local jumps within the block
	if (!tinfo.jump_locations.empty()) {
		emit.add_code("jump_table:");
		emit.add_code("  switch (pc) {");
		for (address_t jump_target : tinfo.jump_locations) {
			if (jump_target < tinfo.basepc || jump_target >= tinfo.endpc)
				throw MachineException(ILLEGAL_OPERATION,
					"emit: jump target outside block", jump_target);
			char label[64];
			snprintf(label, sizeof(label), "  case 0x%" PRIx64 ": goto label_%" PRIx64 ";",
				(uint64_t)jump_target, (uint64_t)jump_target);
			emit.add_code(label);
			mappings.push_back({jump_target, emit.func_name, 0});
		}
		for (address_t jump_target : tinfo.global_jump_locations) {
			if (jump_target < tinfo.basepc || jump_target >= tinfo.endpc)
				continue;
			if (tinfo.jump_locations.count(jump_target) != 0)
				continue; // Already added
			char label[64];
			snprintf(label, sizeof(label), "  case 0x%" PRIx64 ": goto label_%" PRIx64 ";",
				(uint64_t)jump_target, (uint64_t)jump_target);
			emit.add_code(label);
			mappings.push_back({jump_target, emit.func_name, 0});
		}
		// Unknown PC: Return to caller
		emit.add_code("  default:");
		emit.emit_custom_trace("pc"); // Trace unknown jump
		emit.add_code("    cpu->pc = pc; return (ReturnValues){ic, max_ic};");
		emit.add_code("  }");
		emit.add_code("");
	}

	// Process each instruction
	for (size_t i = 0; i < tinfo.instr.size(); i++) {
		const uint32_t instr_bits = tinfo.instr[i];
		const la_instruction instr{instr_bits};

		// Add label if this is a jump target
		if (tinfo.jump_locations.count(emit.pc()) || 
		    tinfo.global_jump_locations.count(emit.pc()))
		{
			// Flush instruction counter at jump targets
			if (emit.pc() != tinfo.basepc)
				emit.flush_instruction_counter();
			char label[64];
			snprintf(label, sizeof(label), "label_%" PRIx64 ":", (uint64_t)emit.pc());
			emit.add_code(label);
		}

		// Emit trace call if tracing is enabled
		emit.emit_trace(instr_bits);
		emit.increment_counter();

		const Instruction decoded = CPU::decode(instr);
		const InstrId instr_id = decoded.id;

		switch (instr_id) {
		// Branch instructions
		case InstrId::BEQZ: {
			int64_t offs = InstructionHelpers::sign_extend_21(instr.ri21.offs_lo, instr.ri21.offs_hi);
			address_t target = emit.pc() + (offs << 2);
			emit.emit_branch_1r("== 0", instr.ri21.rj, target);
			break;
		}
		case InstrId::BNEZ: {
			int64_t offs = InstructionHelpers::sign_extend_21(instr.ri21.offs_lo, instr.ri21.offs_hi);
			address_t target = emit.pc() + (offs << 2);
			emit.emit_branch_1r("!= 0", instr.ri21.rj, target);
			break;
		}
		case InstrId::B: {
			int64_t offs = InstructionHelpers::sign_extend_26(instr.i26.offs());
			address_t target = emit.pc() + (offs << 2);
			emit.emit_jump(target);
			break;
		}
		case InstrId::BL: {
			int64_t offs = InstructionHelpers::sign_extend_26(instr.i26.offs());
			address_t target = emit.pc() + (offs << 2);
			address_t return_addr = emit.pc() + 4;
			emit.emit_call(1, target, return_addr); // rd=1 (ra) for BL
			break;
		}
		case InstrId::BEQ: {
			int64_t offs = InstructionHelpers::sign_extend_16(instr.ri16.imm);
			address_t target = emit.pc() + (offs << 2);
			emit.emit_branch_2r("==", false, instr.ri16.rd, instr.ri16.rj, target);
			break;
		}
		case InstrId::BNE: {
			int64_t offs = InstructionHelpers::sign_extend_16(instr.ri16.imm);
			address_t target = emit.pc() + (offs << 2);
			emit.emit_branch_2r("!=", false, instr.ri16.rd, instr.ri16.rj, target);
			break;
		}
		case InstrId::BLT: {
			int64_t offs = InstructionHelpers::sign_extend_16(instr.ri16.imm);
			address_t target = emit.pc() + (offs << 2);
			emit.emit_branch_2r("<", true, instr.ri16.rd, instr.ri16.rj, target);
			break;
		}
		case InstrId::BGE: {
			int64_t offs = InstructionHelpers::sign_extend_16(instr.ri16.imm);
			address_t target = emit.pc() + (offs << 2);
			emit.emit_branch_2r(">=", true, instr.ri16.rd, instr.ri16.rj, target);
			break;
		}
		case InstrId::BLTU: {
			int64_t offs = InstructionHelpers::sign_extend_16(instr.ri16.imm);
			address_t target = emit.pc() + (offs << 2);
			emit.emit_branch_2r("<", false, instr.ri16.rd, instr.ri16.rj, target);
			break;
		}
		case InstrId::BGEU: {
			int64_t offs = InstructionHelpers::sign_extend_16(instr.ri16.imm);
			address_t target = emit.pc() + (offs << 2);
			emit.emit_branch_2r(">=", false, instr.ri16.rd, instr.ri16.rj, target);
			break;
		}
		case InstrId::BCEQZ:
		case InstrId::BCNEZ: {
			// XXX: Unimplemented condition codes
			emit.add_code("  cpu->pc = " + hex_address(emit.pc()) + "LL;");
			emit.emit_return();
			break;
		}
		case InstrId::JIRL: {
			int64_t offs = InstructionHelpers::sign_extend_16(instr.ri16.imm);
			address_t return_addr = emit.pc() + 4;
			emit.emit_jirl(instr.ri16.rd, instr.ri16.rj, offs << 2, return_addr);
			break;
		}

		// PC-relative instructions
		case InstrId::PCADDI: {
			if (instr.ri20.rd != 0) {
				int32_t si20 = InstructionHelpers::sign_extend_20(instr.ri20.imm);
				int64_t offset = (int64_t)(si20 << 2);
				emit.add_code("  " + emit.reg(instr.ri20.rd) + " = " +
					hex_address(emit.pc() + offset) + "ULL;");
			}
			break;
		}
		case InstrId::PCADDU12I: {
			if (instr.ri20.rd != 0) {
				int64_t si20 = InstructionHelpers::sign_extend_20(instr.ri20.imm);
				int64_t offset = si20 << 12;
				emit.add_code("  " + emit.reg(instr.ri20.rd) + " = " +
					hex_address(emit.pc() + offset) + "ULL;");
			}
			break;
		}
		case InstrId::PCALAU12I: {
			if (instr.ri20.rd != 0) {
				address_t pc_aligned = emit.pc() & ~((address_t)0xFFF);
				int64_t offset = (int64_t)(int32_t)(instr.ri20.imm << 12);
				emit.add_code("  " + emit.reg(instr.ri20.rd) + " = " +
					hex_address(pc_aligned + offset) + "ULL;");
			}
			break;
		}
		case InstrId::PCADDU18I: {
			if (instr.ri20.rd != 0) {
				int32_t si20 = InstructionHelpers::sign_extend_20(instr.ri20.imm);
				int64_t offset = (int64_t)(si20 << 18);
				emit.add_code("  " + emit.reg(instr.ri20.rd) + " = " +
					hex_address(emit.pc() + offset) + "ULL;");
			}
			break;
		}

		// System instructions
		case InstrId::SYSCALL:
			emit.flush_instruction_counter();
			emit.add_code("  cpu->pc = " + hex_address(emit.pc()) + "ULL;");
			emit.add_code("  if (do_syscall(cpu, ic, max_ic, " + emit.reg(REG_A7) + ")) {");
			if (!tinfo.options.translate_ignore_instruction_limit) {
				emit.add_code("    cpu->pc += 4; return (ReturnValues){ic, MAX_COUNTER(cpu)}; }");
				emit.add_code("  max_ic = MAX_COUNTER(cpu);");
			} else {
				emit.add_code("    cpu->pc += 4; return (ReturnValues){0, MAX_COUNTER(cpu)}; }");
			}
			break;

		// Load upper immediate
		case InstrId::LU12I_W:
			if (instr.ri20.rd != 0) {
				int32_t result = (int32_t)(instr.ri20.imm << 12);
				emit.add_code("  " + emit.reg(instr.ri20.rd) + " = " +
					std::to_string((saddress_t)result) + "LL;");
			}
			break;

		// Arithmetic immediate
		case InstrId::ADDI_D: {
			if (instr.ri12.rd != 0) {
				emit.add_code("  " + emit.reg(instr.ri12.rd) + " = " +
					emit.reg(instr.ri12.rj) + " + " + std::to_string(InstructionHelpers::sign_extend_12(instr.ri12.imm)) + ";");
			}
			break;
		}
		case InstrId::ADDI_W: {
			if (instr.ri12.rd != 0) {
				emit.add_code("  " + emit.reg(instr.ri12.rd) + " = (int32_t)" +
					emit.reg(instr.ri12.rj) + " + " + std::to_string(InstructionHelpers::sign_extend_12(instr.ri12.imm)) + ";");
			}
			break;
		}

		// Load/Store
		case InstrId::LD_D: {
			if (instr.ri12.rd != 0) {
				const int32_t si12 = InstructionHelpers::sign_extend_12(instr.ri12.imm);
				std::string addr = emit.reg(instr.ri12.rj) + " + " + std::to_string(si12);
				emit.add_code("  " + emit.reg(instr.ri12.rd) + " = rd64(cpu, " + addr + ");");
			}
			break;
		}
		case InstrId::ST_D: {
			const int32_t si12 = InstructionHelpers::sign_extend_12(instr.ri12.imm);
			std::string addr = emit.reg(instr.ri12.rj) + " + " + std::to_string(si12);
			emit.add_code("  wr64(cpu, " + addr + ", " + emit.reg(instr.ri12.rd) + ");");
			break;
		}

		// Arithmetic register
		case InstrId::ADD_D:
			if (instr.r3.rd != 0) {
				emit.add_code("  " + emit.reg(instr.r3.rd) + " = " +
					emit.reg(instr.r3.rj) + " + " + emit.reg(instr.r3.rk) + ";");
			}
			break;

		// Logical operations
		case InstrId::OR:
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
			break;

		case InstrId::ORI:
			if (instr.ri12.rd != 0) {
				const uint32_t imm12 = instr.ri12.imm;
				emit.add_code("  " + emit.reg(instr.ri12.rd) + " = " +
					emit.reg(instr.ri12.rj) + " | " + std::to_string(imm12) + ";");
			}
			break;

		default:
			// Instruction not handled in binary translator
			emit.emit_fallback(decoded, instr_bits);
		}

		emit.advance_pc();
	}

	emit.add_code("  cpu->pc = " + hex_address(tinfo.endpc) + ";");
	emit.emit_return();
	emit.add_code("}");

	// Append the generated code
	code += emit.code;

	return mappings;
}

} // loongarch
