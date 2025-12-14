#pragma once
#include "common.hpp"
#include <cstdint>

namespace loongarch
{
	struct DecodedExecuteSegment;
	struct MachineOptions;

	// Embedded binary translation metadata
	// This is used to register pre-compiled binary translations
	// that can be embedded directly into the host application
	struct EmbeddedTranslation {
		uint32_t crc32c_hash;              // CRC32-C of execute segment
		void* (*init_func)();              // Returns dylib handle (for embedded, returns the mappings)
		const char* version;               // Libloong version this was compiled for
	};

	// Maximum number of embedded translations
	// This is a fixed-size array to avoid global constructors
	static constexpr size_t MAX_EMBEDDED_TRANSLATIONS = 16;

	// Registry for embedded binary translations
	// This structure is designed to be zero-initialized from BSS
	// to avoid global constructor issues
	struct EmbeddedTranslationRegistry {
		EmbeddedTranslation translations[MAX_EMBEDDED_TRANSLATIONS];
		uint32_t count; // Number of registered translations

		// Thread-safe registration (called by global constructors)
		bool register_translation(uint32_t crc32c, void* (*init)(), const char* version);

		// Lookup a translation by CRC32-C hash
		const EmbeddedTranslation* find(uint32_t crc32c_hash) const;
	};

	// Global registry instance
	// Declared here, defined in tr_embedded.cpp
	extern EmbeddedTranslationRegistry g_embedded_translations;

	// Register an embedded translation
	// This is called by global constructors in embedded translation code
	// Returns true if registration succeeded, false if registry is full
	bool register_embedded_translation(uint32_t crc32c, void* (*init)(), const char* version = "1.0");

	// Try to load and activate an embedded translation
	// Returns true if an embedded translation was found and activated
	bool try_activate_embedded_translation(const MachineOptions& options, DecodedExecuteSegment& exec,
		uint32_t crc32c_hash, void* machine_ptr);

	// Generate embedded translation footer for output file
	// This creates the self-registration code for embedded translations
	std::string generate_embedded_footer(uint32_t crc32c_hash);

} // namespace loongarch
