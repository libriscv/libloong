#pragma once
#include "common.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <string>

namespace loongarch
{
	struct CPU;
	struct DecodedExecuteSegment;

	// Binary translation return values (instruction counter and max counter)
	struct bintr_block_returns {
		uint64_t ic;      // Current instruction counter
		uint64_t max_ic;  // Maximum instruction counter
	};

	// Binary translation block function signature
	// Takes: CPU&, ic (instruction counter), max_ic, pc
	// Returns: bintr_block_returns
	using bintr_block_func = bintr_block_returns (*)(CPU&, uint64_t, uint64_t, address_t);

	// Mapping from virtual address to handler index
	template <typename Addr = address_t>
	struct TransMapping {
		Addr addr;
		std::string symbol;  // Function symbol name
		unsigned mapping_index = 0;
	};

	// Information about a single translated instruction
	struct TransInstr {
		uint32_t instr;  // The instruction bits (use uint32_t to avoid circular dependency)
		address_t pc;          // PC of this instruction
		bool is_branch = false;
		bool is_function_call = false;
	};

	// Information needed for translation
	struct TransInfo {
		std::vector<uint32_t> instr;  // Instructions to translate
		address_t basepc;                    // Base PC of this block
		address_t endpc;                     // End PC of this block
		address_t segment_basepc;            // Segment base PC
		address_t segment_endpc;             // Segment end PC
		bool is_libtcc;                      // Using libtcc (vs system compiler)
		const MachineOptions& options;       // Translation options
		std::unordered_set<address_t> jump_locations;  // Jump targets within block
		// Pointer to all blocks (including current)
		std::vector<TransInfo>* blocks = nullptr;
		std::unordered_set<address_t>& global_jump_locations;

		uintptr_t arena_ptr;     // Pointer to memory arena
		address_t arena_roend;   // End of read-only region
		address_t arena_size;    // Total arena size
	};

	// Output from translation process
	struct TransOutput {
		std::unordered_map<std::string, std::string> defines;
		std::shared_ptr<std::string> code;
		std::string footer;
		std::vector<TransMapping<>> mappings;
	};

} // namespace loongarch
