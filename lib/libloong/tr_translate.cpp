#include "machine.hpp"
#include "decoder_cache.hpp"
#include "la_instr.hpp"
#include "tr_api.hpp"
#include "tr_types.hpp"
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <fstream>

namespace loongarch
{
	// Forward declaration of emit function from tr_emit.cpp
	std::vector<TransMapping<>> emit(std::string& code, const TransInfo& tinfo);

	// Enable verbose block output via VERBOSE=1 environment variable
	// This will print:
	// - Detailed information about each block found during translation
	// - Generated C code for binary translation
	// Usage: VERBOSE=1 ./laemu program.elf
	static bool verbose_blocks() {
		static bool checked = false;
		static bool result = false;
		if (!checked) {
			result = getenv("VERBOSE") != nullptr;
			checked = true;
		}
		return result;
	}

	// Helper to read instruction from execute segment via memory
	static inline la_instruction read_instruction(const Memory& memory, address_t pc, address_t end_pc) {
		if (pc + 4 > end_pc)
			throw MachineException(ILLEGAL_OPERATION, "Reading instruction outside execute segment");
		return la_instruction(memory.read<uint32_t>(pc));
	}

	// Check if instruction is a stopping instruction (ends a block)
	static bool is_stopping_instruction(la_instruction instr) {
		const uint32_t opcode = instr.opcode() & 0xFC000000;

		// JIRL (indirect jump) is a stopping instruction
		if (opcode == 0x4C000000) {
			return true;
		}

		// SYSCALL is a stopping instruction
		if ((instr.opcode() & 0xFFFFFFFF) == 0x002B0000) {
			return true;
		}

		// BREAK is a stopping instruction
		if ((instr.opcode() & 0xFFFFFFFF) == 0x002A0000) {
			return true;
		}

		return false;
	}

	// Check if instruction is a branch
	static bool is_branch_instruction(la_instruction instr, address_t& target, address_t pc) {
		const uint32_t opcode = instr.opcode() & 0xFC000000;

		// BEQZ, BNEZ (1RI21 format)
		if (opcode == 0x40000000 || opcode == 0x44000000) {
			// Sign-extend 21-bit offset, scale by 4
			int32_t offs = (int32_t)(instr.ri21.offs() << 11) >> 9; // Sign extend and shift left by 2
			target = pc + offs;
			return true;
		}

		// BEQ, BNE, BLT, BGE, BLTU, BGEU (2RI16 format)
		if (opcode >= 0x58000000 && opcode <= 0x6C000000) {
			// Sign-extend 16-bit offset, scale by 4
			int32_t offs = (int32_t)(int16_t)(instr.ri16.imm) << 2;
			target = pc + offs;
			return true;
		}

		return false;
	}

	// Check if instruction is a direct jump (JAL/JIRL with known target)
	static bool is_direct_jump(la_instruction instr, address_t& target, address_t pc, bool& is_call) {
		const uint32_t opcode = instr.opcode() & 0xFC000000;

		// B (unconditional branch, I26 format)
		if (opcode == 0x50000000) {
			// Sign-extend 26-bit offset, scale by 4
			int32_t offs = (int32_t)(instr.i26.offs() << 6) >> 4; // Sign extend and shift left by 2
			target = pc + offs;
			is_call = false;
			return true;
		}

		// BL (branch and link, I26 format)
		if (opcode == 0x54000000) {
			// Sign-extend 26-bit offset, scale by 4
			int32_t offs = (int32_t)(instr.i26.offs() << 6) >> 4; // Sign extend and shift left by 2
			target = pc + offs;
			is_call = true;
			return true;
		}

		// JIRL with rd=0, rj!=1 and imm=0 could be a jump (but usually indirect)
		// We only handle JIRL as stopping for now

		return false;
	}

	void binary_translate(const Machine& machine, const MachineOptions& options,
		DecodedExecuteSegment& exec, TransOutput& output)
	{
		const bool verbose = options.verbose_loader;
		const bool is_libtcc = true; // We always use libtcc for LoongArch

		const address_t basepc = exec.exec_begin();
		const address_t endbasepc = exec.exec_end();
		const uintptr_t arena_ptr = (uintptr_t)machine.memory.arena_ptr();
		const address_t arena_size = machine.memory.arena_size();
		const address_t arena_roend = 0; // Flat arena, no special read-only end

		// Code block detection
		const size_t ITS_TIME_TO_SPLIT = is_libtcc ? 5'000 : 1'250;
		size_t icounter = 0;
		std::unordered_set<address_t> global_jump_locations;
		std::vector<TransInfo> blocks;

		// Insert the ELF entry point as the first global jump location
		const auto elf_entry = machine.memory.start_address();
		if (elf_entry >= basepc && elf_entry < endbasepc)
			global_jump_locations.insert(elf_entry);
		// Speculate that the first instruction is a jump target
		global_jump_locations.insert(exec.exec_begin());

		// Scan through execute segment and create blocks
		for (address_t pc = basepc; pc < endbasepc && icounter < options.translate_instr_max; )
		{
			const auto block = pc;
			std::size_t block_insns = 0;

			// Find the end of this block
			for (; pc < endbasepc; ) {
				const la_instruction instruction = read_instruction(machine.memory, pc, endbasepc);
				pc += 4; // All LoongArch instructions are 4 bytes
				block_insns++;

				// JIRL, SYSCALL, BREAK are show-stoppers / code-block enders
				if (block_insns >= ITS_TIME_TO_SPLIT && is_stopping_instruction(instruction)) {
					break;
				}
			}

			auto block_end = pc;
			std::unordered_set<address_t> jump_locations;
			std::vector<uint32_t> block_instructions;
			block_instructions.reserve(block_insns);

			// Find jump locations inside block
			for (pc = block; pc < block_end; ) {
				const la_instruction instruction = read_instruction(machine.memory, pc, endbasepc);

				address_t location = 0;
				bool is_call = false;

				// Check for direct jumps (B, BL)
				if (is_direct_jump(instruction, location, pc, is_call)) {

					// All JAL target addresses need to be recorded
					global_jump_locations.insert(location);

					// Record return location for calls
					if (is_call) {
						global_jump_locations.insert(pc + 4);
					}

					// If jump target is within current block, record as local jump
					if (location >= block && location < block_end)
						jump_locations.insert(location);
				}
				// Check for conditional branches
				else if (is_branch_instruction(instruction, location, pc)) {
					// Only accept branches relative to current block
					if (location >= block && location < block_end)
						jump_locations.insert(location);
					else
						global_jump_locations.insert(location);
				}

				// Add instruction to block
				block_instructions.push_back(instruction.whole);
				pc += 4;
			}

			// Process block and add it for emission
			const size_t length = block_instructions.size();
			if (length > 0 && icounter + length < options.translate_instr_max)
			{
				if (verbose_blocks()) {
					printf("Block found at %#lX -> %#lX. Length: %zu\n", long(block), long(block_end), length);
					printf("  Local jump locations within block:\n");
					for (auto loc : jump_locations)
						printf("    -> %#lX\n", long(loc));
					printf("  First instruction: 0x%08X\n", block_instructions.empty() ? 0 : block_instructions[0]);
					printf("  Last instruction:  0x%08X\n", block_instructions.empty() ? 0 : block_instructions[length - 1]);
				}

				blocks.push_back({
					std::move(block_instructions),
					block, block_end,
					basepc, endbasepc,
					is_libtcc,
					options.translate_trace,
					options.translate_ignore_instruction_limit,
					options.use_shared_execute_segments,
					options.translate_use_register_caching,
					options.unsafe_remove_checks,
					std::move(jump_locations),
					nullptr, // blocks pointer (set below)
					global_jump_locations,
					arena_ptr,
					arena_roend,
					arena_size
				});
				icounter += length;

				// We can't translate beyond this estimate, otherwise
				// the compiler will never finish code generation
				if (blocks.size() >= options.translate_blocks_max)
					break;
			}

			pc = block_end;
		}

		// Code generation will be implemented in tr_emit.cpp
		// For now, just set up the output structure
		output.code = std::make_shared<std::string>();
		auto& code = *output.code;

		// Add header with API inclusion
		extern const std::string bintr_code;
		code = bintr_code;

		// Generate code for each block
		for (auto& block : blocks)
		{
			block.blocks = &blocks;
			auto result = emit(code, block);
			for (auto& mapping : result) {
				output.mappings.push_back(std::move(mapping));
			}
		}

		// Write generated code to output file if specified
		if (!options.translate_output_file.empty() && !code.empty()) {
			std::ofstream ofs(options.translate_output_file, std::ios::out | std::ios::trunc);
			if (ofs.is_open()) {
				ofs << code;
				ofs.close();
				if (verbose) {
					printf("libloong: Generated translation code written to %s\n",
						options.translate_output_file.c_str());
				}
			} else {
				fprintf(stderr, "libloong: Failed to write translation code to %s\n",
					options.translate_output_file.c_str());
			}
		}

		// Generate footer for shared libraries
		output.footer += "VISIBLE const uint32_t no_mappings = "
			+ std::to_string(output.mappings.size()) + ";\n";
		output.footer += R"V0G0N(
struct Mapping {
	addr_t   addr;
	unsigned mapping_index;
};
VISIBLE const struct Mapping mappings[] = {
)V0G0N";

		std::unordered_map<std::string, unsigned> mapping_indices;
		std::vector<const std::string*> handlers;
		handlers.reserve(blocks.size());

		for (const auto& mapping : output.mappings)
		{
			// Create map of unique mappings
			unsigned mapping_index = 0;
			auto it = mapping_indices.find(mapping.symbol);
			if (it == mapping_indices.end()) {
				mapping_index = handlers.size();
				mapping_indices.emplace(mapping.symbol, mapping_index);
				handlers.push_back(&mapping.symbol);
			} else {
				mapping_index = it->second;
			}

			char buffer[128];
			snprintf(buffer, sizeof(buffer),
				"{0x%lX, %u},\n",
				(long)mapping.addr, mapping_index);
			output.footer.append(buffer);
		}

		output.footer += "};\nVISIBLE const uint32_t no_handlers = "
			+ std::to_string(mapping_indices.size()) + ";\n"
			+ "VISIBLE const void* unique_mappings[] = {\n";

		// Create array of unique mappings
		for (auto* handler : handlers) {
			output.footer += "    " + *handler + ",\n";
		}
		output.footer += "};\n";

		if (verbose) {
			printf("libloong: Binary translation summary:\n");
			printf("  - Translated %zu instructions across %zu blocks\n", icounter, blocks.size());
			printf("  - Generated %zu function mappings\n", output.mappings.size());
			printf("  - Execute segment: 0x%lX - 0x%lX (%zu bytes)\n",
				(long)basepc, (long)endbasepc, (size_t)(endbasepc - basepc));
			printf("  - Global jump targets: %zu\n", global_jump_locations.size());
			printf("  - Trace enabled: %s\n", options.translate_trace ? "yes" : "no");
		}
	}

	// Simplified compilation for libtcc
	// This compiles the generated C code using libtcc and returns a "dylib" handle
	#ifdef LA_BINARY_TRANSLATION
	void* compile_with_libtcc(const std::string& code, const MachineOptions& options)
	{
		#ifdef ENABLE_LIBTCC
		extern void* libtcc_compile(const std::string&, int arch, const std::unordered_map<std::string, std::string>& defines, const std::string&);

		// Create defines map (empty for now, can be extended later)
		std::unordered_map<std::string, std::string> defines;

		if (options.verbose_loader && verbose_blocks()) {
			// Write generated code to file for debugging
			std::ofstream ofs("libtcc_output_loongarch.c", std::ios::out | std::ios::trunc);
			if (ofs.is_open()) {
				ofs << code;
				ofs.close();
			}
		}

		// Compile with libtcc
		// arch = 64 for 64-bit LoongArch (matching XLEN)
		void* dylib = libtcc_compile(code, 64, defines, "");

		if (dylib == nullptr && options.verbose_loader) {
			fprintf(stderr, "libloong: libtcc compilation failed\n");
		}

		return dylib;
		#else
		(void)code;
		(void)options;
		return nullptr;
		#endif
	}
	#endif

} // namespace loongarch
