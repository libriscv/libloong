#include "tr_embedded.hpp"
#include "tr_internal.hpp"
#include "machine.hpp"
#include "decoder_cache.hpp"
#include <cstring>
#include <atomic>

namespace loongarch
{
	// Global registry instance - zero-initialized from BSS
	EmbeddedTranslationRegistry g_embedded_translations = {};

	bool EmbeddedTranslationRegistry::register_translation(uint32_t crc32c, void* (*init)(), const char* version)
	{
		// Use atomic compare-exchange to safely increment count
		// This allows multiple threads to register simultaneously during global construction
		for (uint32_t i = 0; i < MAX_EMBEDDED_TRANSLATIONS; ++i) {
			uint32_t expected = i;
			if (count == expected) {
				// Try to claim this slot
				// Note: This is technically a data race but acceptable for global construction
				// In practice, global constructors run sequentially on most platforms
				if (i < MAX_EMBEDDED_TRANSLATIONS) {
					translations[i].crc32c_hash = crc32c;
					translations[i].init_func = init;
					translations[i].version = version;

					// Atomically update count only if it's still what we expect
					uint32_t old_count = expected;
					__atomic_compare_exchange_n(&count, &old_count, i + 1,
						false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
					return true;
				}
			}
		}
		return false; // Registry full
	}

	const EmbeddedTranslation* EmbeddedTranslationRegistry::find(uint32_t crc32c_hash) const
	{
		// Read count with memory fence to ensure we see all registered translations
		uint32_t num_translations = __atomic_load_n(&count, __ATOMIC_SEQ_CST);

		for (uint32_t i = 0; i < num_translations && i < MAX_EMBEDDED_TRANSLATIONS; ++i) {
			if (translations[i].crc32c_hash == crc32c_hash) {
				return &translations[i];
			}
		}
		return nullptr;
	}

	bool register_embedded_translation(uint32_t crc32c, void* (*init)(), const char* version)
	{
		return g_embedded_translations.register_translation(crc32c, init, version);
	}

	bool try_activate_embedded_translation(const MachineOptions& options, DecodedExecuteSegment& exec,
		uint32_t crc32c_hash, void* machine_ptr)
	{
		// Look up the embedded translation
		const auto* translation = g_embedded_translations.find(crc32c_hash);
		if (!translation) {
			if (options.verbose_loader) {
				printf("libloong: No embedded translation found for CRC32-C 0x%08X\n", crc32c_hash);
			}
			return false;
		}

		if (options.verbose_loader) {
			printf("libloong: Found embedded translation for CRC32-C 0x%08X (version: %s)\n",
				crc32c_hash, translation->version ? translation->version : "unknown");
		}

		// Call the init function to get the dylib structure
		void* dylib_data = translation->init_func();
		if (!dylib_data) {
			if (options.verbose_loader) {
				fprintf(stderr, "libloong: Embedded translation init function returned nullptr\n");
			}
			return false;
		}

		// Prepare arena information
		auto* machine = static_cast<Machine*>(machine_ptr);
		ArenaInfo arena_info;
		auto* arena_ptr = machine->memory.arena_ref();
		arena_info.arena_ptr = (const uint8_t*)arena_ptr;
		arena_info.arena_offset = reinterpret_cast<intptr_t>((intptr_t)arena_ptr - (intptr_t)machine);
		arena_info.ic_offset = Machine::counter_offset();

		try {
			// Activate the embedded translation
			// is_libtcc=false because this is embedded (not JIT compiled)
			// live_patch=false because embedded translations are loaded at startup
			activate_dylib(options, exec, dylib_data, arena_info, false, false);

			if (options.verbose_loader) {
				printf("libloong: Successfully activated embedded translation\n");
			}

			return true;
		} catch (const std::exception& e) {
			if (options.verbose_loader) {
				fprintf(stderr, "libloong: Failed to activate embedded translation: %s\n", e.what());
			}
			return false;
		}
	}

	std::string generate_embedded_footer(uint32_t crc32c_hash)
	{
		std::string footer;

		footer += R"V0G0N(
// Embedded dylib structure
typedef ReturnValues (*HandlerFunc)(CPU*, uint64_t, uint64_t, addr_t);
struct EmbeddedDylib {
	void (*init_ptr)(struct CallbackTable*, int32_t, int32_t);
	const uint32_t* no_mappings_ptr;
	const struct Mapping* mappings_ptr;
	const uint32_t* no_handlers_ptr;
	const HandlerFunc* unique_mappings_ptr;
};

static struct EmbeddedDylib embedded_dylib = {
	&init,
	&no_mappings,
	&mappings[0],
	&no_handlers,
	&unique_mappings[0]
};

// Embedded translation init function
VISIBLE void* embedded_init(void) {
	return (void*)&embedded_dylib;
}

// Self-registration for embedded translations
)V0G0N";

		// Add the CRC32-C hash
		char crc_buffer[256];
		snprintf(crc_buffer, sizeof(crc_buffer),
			"#define EMBEDDED_CRC32C 0x%08XU\n", crc32c_hash);
		footer += crc_buffer;

		footer += R"V0G0N(
#ifdef __cplusplus
extern "C" {
#endif
extern int loongarch_register_embedded_translation(uint32_t, void* (*)(), const char*);
#ifdef __cplusplus
}
#endif

// Global constructor to register this embedded translation
__attribute__((constructor))
static void register_this_translation(void) {
	loongarch_register_embedded_translation(EMBEDDED_CRC32C, embedded_init, "1.0");
}
)V0G0N";

		return footer;
	}

} // namespace loongarch

// C API for registration (used by embedded translations)
extern "C" {
	int loongarch_register_embedded_translation(uint32_t crc32c, void* (*init)(), const char* version) {
		return loongarch::register_embedded_translation(crc32c, init, version) ? 1 : 0;
	}
}
