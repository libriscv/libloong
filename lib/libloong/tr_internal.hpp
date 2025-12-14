#pragma once
#include "common.hpp"
#include "tr_types.hpp"

namespace loongarch
{
	struct DecodedExecuteSegment;
	struct MachineOptions;

	// Mapping structure from dylib
	struct Mapping {
		address_t addr;
		unsigned mapping_index;
	};

	// Arena information for compilation
	struct ArenaInfo {
		const uint8_t* arena_ptr;
		int32_t arena_offset;
		int32_t ic_offset;
	};

	// Embedded dylib structure (must match tr_translate.cpp output)
	struct EmbeddedDylib {
		void (*init_ptr)(void*, int32_t, int32_t);
		const uint32_t* no_mappings_ptr;
		const Mapping* mappings_ptr;
		const uint32_t* no_handlers_ptr;
		const void** unique_mappings_ptr;
	};

	// Dylib helper function
	void* dylib_lookup(void* dylib, const char* name, bool is_libtcc);

	// Initialize a translated segment with callbacks
	bool initialize_translated_segment(DecodedExecuteSegment& exec, void* dylib,
		const ArenaInfo& arena_info, bool is_libtcc);

	// Activate a compiled dylib
	void activate_dylib(MachineOptions options, DecodedExecuteSegment& exec, void* dylib,
		const ArenaInfo& arena_info, bool is_libtcc, bool live_patch);

} // namespace loongarch
