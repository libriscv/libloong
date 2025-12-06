#pragma once
#include <cstdint>
#include <string>

namespace loongarch {
	// Forward declarations
	struct CPU;
	struct Machine;

	// Binary translation API will be added here
	// For now, just declare the bintr_code string that's defined in tr_api.cpp
	extern const std::string bintr_code;

	// Callback table for binary translated code (simplified for flat arena)
	struct CallbackTable {
		// We use flat arena, so memory access is direct
		// No callbacks needed for memory in the simple case

		// System operations
		uint64_t (*system_call)(CPU&, address_t, uint64_t, uint64_t, int);
		void (*unknown_syscall)(CPU&, address_t);
		void (*trigger_exception)(CPU&, address_t, int);

		// Instruction execution fallback for unimplemented instructions
		unsigned (*execute)(CPU&, uint32_t);
		unsigned (*execute_handler)(CPU&, uint32_t, void(*)(CPU&, uint32_t));
		void (**handlers)(CPU&, uint32_t);

		// Tracing support
		void (*trace)(CPU&, const char*, address_t, uint32_t);

		// Math operations that TCC doesn't have
		float  (*sqrtf32)(float);
		double (*sqrtf64)(double);
		int (*clz) (uint32_t);
		int (*clzl) (uint64_t);
		int (*ctz) (uint32_t);
		int (*ctzl) (uint64_t);
		int (*cpop) (uint32_t);
		int (*cpopl) (uint64_t);
	};

} // namespace loongarch
