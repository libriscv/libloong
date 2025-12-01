#include "machine.hpp"
#include "cpu.hpp"
#include "memory.hpp"
#include "threaded_bytecodes.hpp"
#include "decoder_cache.hpp"
#include <unordered_map>
#include <algorithm>

namespace loongarch
{
	std::vector<typename Machine::BytecodeStats> Machine::collect_bytecode_statistics() const
	{
		using handler_t = typename DecoderData::handler_t;

		// Count bytecode usage from main execute segment
		std::unordered_map<uint8_t, uint64_t> bytecode_counts;
		// For fallback bytecodes, track per-handler counts
		std::unordered_map<handler_t, uint64_t> handler_counts;
		std::unordered_map<handler_t, uint32_t> handler_samples;

		// Get the main execute segment
		const auto main_exec = memory.exec_segment_for(memory.start_address());
		if (!main_exec || !main_exec->decoder_cache()) {
			return {}; // No decoder cache available
		}

		// Iterate through the decoder cache
		const auto* cache = main_exec->decoder_cache();
		const size_t cache_size = main_exec->decoder_cache_size();

		for (size_t i = 0; i < cache_size; ++i) {
			const auto& entry = cache[i];
			const uint8_t bc = entry.get_bytecode();

			// Count this bytecode
			bytecode_counts[bc]++;

			// For fallback bytecodes (LA64_BC_FUNCTION),
			// track per-handler statistics so we can show all unimplemented instructions
			if (bc == LA64_BC_FUNCTION) {
				handler_t handler = entry.get_handler();
				handler_counts[handler]++;

				// Store a sample instruction for this handler
				if (handler_samples.find(handler) == handler_samples.end()) {
					handler_samples[handler] = entry.instr;
				}
			}
		}

		// Convert to vector and sort by count (descending)
		std::vector<BytecodeStats> stats;
		stats.reserve(bytecode_counts.size() + handler_counts.size());

		for (const auto& [bytecode, count] : bytecode_counts) {
			// Skip FUNCTION here - we'll add them per-handler below
			if (bytecode == LA64_BC_FUNCTION) {
				continue;
			}

			BytecodeStats stat;
			stat.bytecode = bytecode;
			stat.count = count;
			stat.sample_instruction = 0;
			stats.push_back(stat);
		}

		// Add per-handler statistics for fallback bytecodes
		for (const auto& [handler, count] : handler_counts) {
			BytecodeStats stat;
			stat.bytecode = LA64_BC_FUNCTION;
			stat.count = count;
			stat.sample_instruction = handler_samples[handler];
			stats.push_back(stat);
		}

		// Sort by count (descending)
		std::sort(stats.begin(), stats.end(), [](const BytecodeStats& a, const BytecodeStats& b) {
			return a.count > b.count;
		});

		return stats;
	}

} // loongarch
