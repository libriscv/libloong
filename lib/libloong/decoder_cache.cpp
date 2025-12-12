#include "decoded_exec_segment.hpp"
#include "cpu.hpp"
#include "la_instr.hpp"
#include "threaded_bytecodes.hpp"
#include "util/crc32.hpp"
#include <cstring>

namespace loongarch {
extern bool try_translate(const Machine&, const MachineOptions&, std::shared_ptr<DecodedExecuteSegment>&);

	// Check if an instruction is diverging (changes control flow)
	// Note: PC-reading instructions (PCADDI, PCALAU12I, PCADDU12I) are NOT diverging
	// because they only read PC, they don't modify it
	static bool is_diverging_instruction(uint32_t instr)
	{
		const uint32_t op6 = (instr >> 26) & 0x3F;

		switch (op6) {
		case 0x10: // BEQZ (0x40000000)
		case 0x11: // BNEZ (0x44000000)
		case 0x12: // BCEQZ/BCNEZ (0x48000xxx)
		case 0x13: // JIRL (0x4C000000)
		case 0x14: // B (0x50000000)
		case 0x15: // BL (0x54000000)
		case 0x16: // BEQ (0x58000000)
		case 0x17: // BNE (0x5C000000)
		case 0x18: // BLT (0x60000000)
		case 0x19: // BGE (0x64000000)
		case 0x1A: // BLTU (0x68000000)
		case 0x1B: // BGEU (0x6C000000)
			return true;
		}

		// System calls (exact match required)
		if (instr == Opcode::SYSCALL) return true;
		if (instr == Opcode::BREAK) return true;

		return false;
	}

	// Determine the bytecode for a given instruction
	// Determine the bytecode for a given instruction using its ID
	static uint8_t determine_bytecode(InstrId id, const uint32_t instr, const uint16_t handler_idx)
	{
		// Use switch on instruction ID enum instead of re-decoding
		// This maps InstrId to bytecode, avoiding duplicate decoding logic
		switch (id) {
		// System calls
		case InstrId::SYSCALL:
			return LA64_BC_SYSCALL;
		case InstrId::BREAK:
			return LA64_BC_FUNCTION;
		case InstrId::NOP:
			return LA64_BC_NOP;

		// Special handling for MOVE pseudo-instruction
		case InstrId::OR:
			// MOVE is OR with rj==0
			{
				const uint32_t rj = (instr >> 5) & 0x1F;
				if (rj == 0) {
					return LA64_BC_MOVE;
				}
				return LA64_BC_OR;
			}
		case InstrId::ORI:
			// MOVE is ORI with imm==0
			{
				const uint32_t imm = instr & 0xFFF;
				if (imm == 0) {
					return LA64_BC_MOVE;
				}
				return LA64_BC_ORI;
			}

		case InstrId::LD_D: return LA64_BC_LD_D;
		case InstrId::ST_D: return LA64_BC_ST_D;
		case InstrId::ADDI_W: return LA64_BC_ADDI_W;
		case InstrId::ADDI_D: return LA64_BC_ADDI_D;
		case InstrId::AND: return LA64_BC_AND;
		case InstrId::ANDI: return LA64_BC_ANDI;
		case InstrId::ADD_D: return LA64_BC_ADD_D;
		case InstrId::SUB_D: return LA64_BC_SUB_D;
		case InstrId::SLLI_W: return LA64_BC_SLLI_W;
		case InstrId::SLLI_D: return LA64_BC_SLLI_D;
		case InstrId::SRLI_D: return LA64_BC_SRLI_D;
		case InstrId::LD_BU: return LA64_BC_LD_BU;
		case InstrId::ST_B: return LA64_BC_ST_B;
		case InstrId::ST_W: return LA64_BC_ST_W;
		case InstrId::LDPTR_D: return LA64_BC_LDPTR_D;
		case InstrId::LDPTR_W: return LA64_BC_LDPTR_W;
		case InstrId::STPTR_D: return LA64_BC_STPTR_D;
		case InstrId::LU12I_W: return LA64_BC_LU12I_W;
		case InstrId::BSTRPICK_D: return LA64_BC_BSTRPICK_D;
		case InstrId::ALSL_D: return LA64_BC_ALSL_D;
		case InstrId::LD_B: return LA64_BC_LD_B;
		case InstrId::STPTR_W: return LA64_BC_STPTR_W;
		case InstrId::LDX_D: return LA64_BC_LDX_D;
		case InstrId::MASKEQZ: return LA64_BC_MASKEQZ;
		case InstrId::MASKNEZ: return LA64_BC_MASKNEZ;
		case InstrId::MUL_D: return LA64_BC_MUL_D;
		case InstrId::SUB_W: return LA64_BC_SUB_W;
		case InstrId::SLL_D: return LA64_BC_SLL_D;
		case InstrId::STX_D: return LA64_BC_STX_D;
		case InstrId::BSTRPICK_W: return LA64_BC_BSTRPICK_W;
		case InstrId::SLTU: return LA64_BC_SLTU;
		case InstrId::LDX_H: return LA64_BC_LDX_H;
		case InstrId::LDX_W: return LA64_BC_LDX_W;
		case InstrId::STX_H: return LA64_BC_STX_H;
		case InstrId::STX_W: return LA64_BC_STX_W;
		case InstrId::XOR: return LA64_BC_XOR;
		case InstrId::LD_HU: return LA64_BC_LD_HU;
		case InstrId::ADD_W: return LA64_BC_ADD_W;
		case InstrId::SRAI_D: return LA64_BC_SRAI_D;
		case InstrId::EXT_W_B: return LA64_BC_EXT_W_B;
		case InstrId::LDX_BU: return LA64_BC_LDX_BU;
		case InstrId::BSTRINS_D: return LA64_BC_BSTRINS_D;
		case InstrId::LU32I_D: return LA64_BC_LU32I_D;
		case InstrId::REVB_2H: return LA64_BC_REVB_2H;
		case InstrId::BYTEPICK_D: return LA64_BC_BYTEPICK_D;
		case InstrId::SLTI: return LA64_BC_SLTI;
		case InstrId::ST_H: return LA64_BC_ST_H;
		case InstrId::FLD_D: return LA64_BC_FLD_D;
		case InstrId::FST_D: return LA64_BC_FST_D;
		case InstrId::FADD_D: return LA64_BC_FADD_D;
		case InstrId::FMUL_D: return LA64_BC_FMUL_D;
		case InstrId::FLDX_D: return LA64_BC_FLDX_D;
		case InstrId::FSTX_D: return LA64_BC_FSTX_D;
		case InstrId::FMADD_D: return LA64_BC_FMADD_D;
		case InstrId::VFMADD_D: return LA64_BC_VFMADD_D;
		case InstrId::VHADDW_D_W: return LA64_BC_VHADDW_D_W;
		case InstrId::XVLD: return LA64_BC_XVLD;
		case InstrId::XVST: return LA64_BC_XVST;
		case InstrId::SRLI_W: return LA64_BC_SRLI_W;
		case InstrId::SRL_D: return LA64_BC_SRL_D;
		case InstrId::LU52I_D: return LA64_BC_LU52I_D;
		case InstrId::XORI: return LA64_BC_XORI;
		case InstrId::SLTUI: return LA64_BC_SLTUI;
		case InstrId::LD_H: return LA64_BC_LD_H;
		case InstrId::LDX_HU: return LA64_BC_LDX_HU;
		case InstrId::LD_WU: return LA64_BC_LD_WU;
		case InstrId::ANDN: return LA64_BC_ANDN;
		case InstrId::STX_B: return LA64_BC_STX_B;
		case InstrId::EXT_W_H: return LA64_BC_EXT_W_H;
		case InstrId::REVB_4H: return LA64_BC_REVB_4H;
		case InstrId::LDX_B: return LA64_BC_LDX_B;
		case InstrId::SLT: return LA64_BC_SLT;
		case InstrId::ORN: return LA64_BC_ORN;
		case InstrId::MUL_W: return LA64_BC_MUL_W;
		case InstrId::MOD_DU: return LA64_BC_MOD_DU;
		case InstrId::VLD: return LA64_BC_VLD;
		case InstrId::VST: return LA64_BC_VST;
		case InstrId::VLDX: return LA64_BC_VLDX;
		case InstrId::VSTX: return LA64_BC_VSTX;
		case InstrId::XVLDX: return LA64_BC_XVLDX;
		case InstrId::XVSTX: return LA64_BC_XVSTX;
		case InstrId::VFADD_D: return LA64_BC_VFADD_D;

		// PC-modifying non-diverging instructions
		case InstrId::PCADDI: return LA64_BC_PCADDI;
		case InstrId::PCALAU12I: return LA64_BC_PCALAU12I;
		case InstrId::PCADDU12I: return LA64_BC_PCADDU12I;
		case InstrId::PCADDU18I: return LA64_BC_PCADDU18I;

		// Branch instructions
		case InstrId::BEQZ: return LA64_BC_BEQZ;
		case InstrId::BNEZ: return LA64_BC_BNEZ;
		case InstrId::BCEQZ: return LA64_BC_BCEQZ;
		case InstrId::BCNEZ: return LA64_BC_BCNEZ;
		case InstrId::BEQ: return LA64_BC_BEQ;
		case InstrId::BNE: return LA64_BC_BNE;
		case InstrId::JIRL: return LA64_BC_JIRL;
		case InstrId::B: return LA64_BC_B;
		case InstrId::BL: return LA64_BC_BL;
		case InstrId::BLT: return LA64_BC_BLT;
		case InstrId::BGE: return LA64_BC_BGE;
		case InstrId::BLTU: return LA64_BC_BLTU;
		case InstrId::BGEU: return LA64_BC_BGEU;

		// All other instructions fall through to FUNCTION
		default:
			return LA64_BC_FUNCTION + (handler_idx >> 8);
		}
	}

	// Populate decoder cache for an execute segment
	void populate_decoder_cache(Machine& machine, const MachineOptions& options, std::shared_ptr<DecodedExecuteSegment>& segment,
		address_t exec_begin, const uint8_t* code, size_t code_size, bool is_initial)
	{
		// Compute and store CRC32-C hash for shared segment identification
		const uint32_t crc = util::crc32c(code, code_size);
		segment->set_crc32c_hash(crc);

		// Round down to nearest instruction boundary (4 bytes)
		// This safely handles segments where .text + .rodata are merged
		const size_t aligned_size = code_size & ~size_t(3);
		if (aligned_size < 4) {
			// No complete instructions to cache
			segment->set_decoder_cache(nullptr, 0);
			return;
		}

		const size_t num_instructions = aligned_size / 4;
		auto* cache = new DecoderData[num_instructions + 1];
		// Guarantee that invalid instruction is handler 0
		const auto invalid_handler = DecoderData::compute_handler_for(
			CPU::get_invalid_instruction().handler);
		if (invalid_handler != 0) {
			// This should never happen, but just in case
			throw std::runtime_error("DecoderCache: Handler 0 is not invalid handler");
		}

		// Scan backwards to calculate block_bytes
		// This computes how many bytes until the next diverging instruction
		const uint32_t* instr_ptr = reinterpret_cast<const uint32_t*>(code);
		uint32_t accumulated_bytes = 0;
		std::unordered_map<typename DecoderData::handler_t, uint16_t> handler_map;
		for (size_t i = num_instructions; i-- > 0; ) {
			const uint32_t instr = instr_ptr[i];

			// Decode and cache the handler for fast dispatch
			const auto& decoded = CPU::decode(la_instruction{instr});
			auto it = handler_map.find(decoded.handler);
			uint16_t handler_idx = 0;
			if (it != handler_map.end()) {
				// Existing handler
				handler_idx = it->second;
			} else {
				// New handler
				handler_idx = DecoderData::compute_handler_for(decoded.handler);
				handler_map.insert_or_assign(decoded.handler, handler_idx);
			}

			// Set bytecode for threaded dispatch
			cache[i].bytecode = determine_bytecode(decoded.id, instr, handler_idx);
			cache[i].handler_idx = handler_idx & 0xFF;
			// Optimize instruction bits for popular bytecodes
			// The optimizer may also modify the bytecode if needed,
			// typically to rewrite cases where rd == zero register.
			// This avoids a check in the hot-path for rd != 0.
			const address_t pc = exec_begin + (i * sizeof(la_instruction));
			cache[i].instr = segment->optimize_bytecode(cache[i].bytecode, pc, instr);

			if (is_diverging_instruction(instr)) {
				// Diverging instruction: block_bytes = 0
				cache[i].block_bytes = 0;
				accumulated_bytes = 0;
			} else {
				// Non-diverging: accumulate bytes to next diverge
				accumulated_bytes += 4;
				cache[i].block_bytes = accumulated_bytes;
			}
		}
		// The final instruction in every segment must be zero (invalid)
		// This marks the end of the cache, and prevents overruns
		cache[num_instructions].instr = 0;
		cache[num_instructions].block_bytes = 0;
		cache[num_instructions].bytecode = LA64_BC_INVALID;
		cache[num_instructions].handler_idx = 0;

		// Store the cache in the segment
		segment->set_decoder_cache(cache, num_instructions);

#ifdef LA_BINARY_TRANSLATION
		// Try to activate binary translation if enabled
		// Note: Binary translation with shared segments requires compatible arena sizes
		if (is_initial && options.translate_enabled) {
			try_translate(machine, options, segment);
		}
#endif
	}

	uint16_t DecoderData::compute_handler_for(handler_t handler)
	{
		// Search for existing handler
		for (size_t i = 0; i < m_handlers.size(); ++i) {
			if (m_handlers[i] == handler) {
				return static_cast<uint16_t>(i);
			}
		}

		// Add new handler
		m_handlers.push_back(handler);
		return static_cast<uint16_t>(m_handlers.size() - 1);
	}

	void DecodedExecuteSegment::set(address_t entry_addr, const DecoderData& data)
	{
		const size_t index = (entry_addr - m_exec_begin) >> DecoderCache::SHIFT;
		if (index < m_decoder_cache.size) {
			m_decoder_cache.cache[index] = data;
		} else {
			fprintf(stderr,
				"DecodedExecuteSegment: set() address out of range: 0x%lx index=%zu size=%zu\n",
				long(entry_addr), index, m_decoder_cache.size);
			throw MachineException(INVALID_PROGRAM,
				"DecodedExecuteSegment: set() address out of range", entry_addr);
		}
	}

} // loongarch
