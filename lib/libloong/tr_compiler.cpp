#include "machine.hpp"
#include "decoder_cache.hpp"
#include "threaded_bytecodes.hpp"
#include "tr_types.hpp"
#include <cstring>
#include <atomic>
#include <mutex>

#ifdef ENABLE_LIBTCC
#include <libtcc.h>
#if defined(__aarch64__) || defined(__riscv) || defined(__loongarch64)
#include <lib-arm64.h>
#else
#include <libtcc1.h>
#endif
#endif

namespace loongarch
{
	// Forward declarations
	void binary_translate(const Machine& machine, const MachineOptions& options,
		DecodedExecuteSegment& exec, TransOutput& output);

#ifdef ENABLE_LIBTCC
	void* libtcc_compile(const std::string& code, const std::unordered_map<std::string, std::string>& defines, const std::string& libtcc1)
	{
		(void)libtcc1;

		TCCState* state = tcc_new();
		if (!state)
			return nullptr;

		tcc_set_output_type(state, TCC_OUTPUT_MEMORY);

		for (const auto& pair : defines) {
			tcc_define_symbol(state, pair.first.c_str(), pair.second.c_str());
		}

		tcc_define_symbol(state, "ARCH", "HOST_UNKNOWN");
		tcc_define_symbol(state, "LA_SYSCALLS_MAX", std::to_string(LA_SYSCALLS_MAX).c_str());
		tcc_define_symbol(state, "LA_MACHINE_ALIGNMENT", "64");
		tcc_set_options(state, "-std=c99 -O2 -nostdlib");

#if defined(_WIN32)
		tcc_add_include_path(state, "win32");
#elif defined(__linux__) && defined(__x86_64__)
		tcc_add_include_path(state, "/usr/include/x86_64-linux-gnu");
#elif defined(__linux__) && defined(__aarch64__)
		tcc_add_include_path(state, "/usr/include/aarch64-linux-gnu");
#elif defined(__linux__) && defined(__riscv)
		tcc_add_include_path(state, "/usr/include/riscv64-linux-gnu");
#elif defined(__linux__) && defined(__loongarch64)
		tcc_add_include_path(state, "/usr/include/loongarch64-linux-gnu");
#endif

		tcc_add_symbol(state, "memset", (void*)memset);
		tcc_add_symbol(state, "memcpy", (void*)memcpy);
		tcc_add_symbol(state, "memcmp", (void*)memcmp);
		tcc_add_symbol(state, "memmove", (void*)memmove);

		std::string code1;
#if defined(__aarch64__) || defined(__riscv) || defined(__loongarch64)
		code1 = std::string((const char*)lib_lib_arm64_c, lib_lib_arm64_c_len) + code;
#else
		code1 = std::string((const char*)lib_libtcc1_c, lib_libtcc1_c_len) + code;
#endif

		if (tcc_compile_string(state, code1.c_str()) < 0) {
			if (getenv("VERBOSE")) {
				fprintf(stderr, "%s\n", code1.c_str());
			}
			tcc_delete(state);
			return nullptr;
		}

#if defined(TCC_RELOCATE_AUTO)
		if (tcc_relocate(state, TCC_RELOCATE_AUTO) < 0)
#else
		if (tcc_relocate(state) < 0)
#endif
		{
			tcc_delete(state);
			return nullptr;
		}

		return state;
	}

	void* tcc_lookup(void* state, const char* symbol)
	{
		return tcc_get_symbol((TCCState*)state, symbol);
	}

	void tcc_close(void* state)
	{
		tcc_delete((TCCState*)state);
	}
#endif

	// Dylib helper functions
	static inline void* dylib_lookup(void* dylib, const char* name, bool is_libtcc)
	{
#ifdef ENABLE_LIBTCC
		if (is_libtcc) {
			return tcc_lookup(dylib, name);
		}
#else
		(void)is_libtcc;
#endif
		// For non-libtcc, we would use dlsym here
		(void)dylib;
		(void)name;
		return nullptr;
	}

	void dylib_close(void* dylib, bool is_libtcc)
	{
#ifdef ENABLE_LIBTCC
		if (is_libtcc) {
			tcc_close(dylib);
			return;
		}
#else
		(void)is_libtcc;
#endif
		// For non-libtcc, we would use dlclose here
		(void)dylib;
	}

	// Mapping structure from dylib
	struct Mapping {
		address_t addr;
		unsigned mapping_index;
	};

	// Arena information for background compilation
	struct ArenaInfo {
		const uint8_t* arena_ptr;
		int32_t arena_offset;
		int32_t ic_offset;
	};

	// Initialize the translated segment with callbacks
	static bool initialize_translated_segment(DecodedExecuteSegment& exec, void* dylib, const ArenaInfo& arena_info, bool is_libtcc)
	{
		if (dylib == nullptr)
			return false;

		// Look up the init function
		auto* init_func = (void(*)(void*, int32_t, int32_t))dylib_lookup(dylib, "init", is_libtcc);
		if (init_func == nullptr)
			return false;

		// Create callback table matching tr_api.cpp structure
		struct ReturnValues {
			uint64_t ic;
			uint64_t max_ic;
		};
		static struct CallbackTable {
			Machine::syscall_t** syscalls;
			Machine::unknown_syscall_t* unknown_syscall;
			DecoderData::handler_t* handlers;
			int  (*syscall)(CPU&, unsigned, uint64_t, address_t);
			ReturnValues (*exception) (CPU&, address_t, address_t, int);
			void (*trace) (CPU&, const char*, address_t, uint32_t);
			void (*log) (CPU&, address_t, const char*);
			void (*fallback) (CPU&, address_t, uint32_t);
			float  (*sqrtf32)(float);
			double (*sqrtf64)(double);
			int (*clz) (uint32_t);
			int (*clzl) (uint64_t);
			int (*ctz) (uint32_t);
			int (*ctzl) (uint64_t);
			int (*cpop) (uint32_t);
			int (*cpopl) (uint64_t);
		} callback_table;

		callback_table.syscalls = Machine::get_syscall_handlers();
		callback_table.unknown_syscall = Machine::get_unknown_syscall_handler();
		callback_table.syscall = [](CPU& cpu, unsigned sysnum, uint64_t max_ic, address_t pc) -> int {
			try {
				cpu.registers().pc = pc;
				cpu.machine().set_max_instructions(max_ic);
				cpu.machine().system_call(sysnum);
				return cpu.machine().stopped() || (cpu.pc() != pc);
			} catch (...) {
				cpu.machine().set_current_exception(std::current_exception());
				cpu.machine().stop();
				return -1; // Indicate error
			}
		};
		callback_table.handlers = DecoderData::get_handlers_array();
		callback_table.exception = [](CPU& cpu, address_t pc, address_t data, int type) -> ReturnValues {
			cpu.registers().pc = pc;
			const char* reason =
				(type == ExceptionType::PROTECTION_FAULT) ? "Protection fault" : "Exception triggered";
			cpu.machine().set_current_exception(
				std::make_exception_ptr(MachineException(static_cast<ExceptionType>(type), reason, data))
			);
			cpu.machine().stop();
			return ReturnValues{ cpu.machine().instruction_counter(), 0u };
		};
		callback_table.trace = [](CPU& cpu, const char* desc, address_t pc, uint32_t instr) {
			char buffer[256];
			(void)cpu.decode(la_instruction{instr}).printer(buffer, sizeof(buffer), cpu, la_instruction{instr}, pc);
			printf("[trace] PC=0x%lx: %s (0x%08x): %s\n", (unsigned long)pc, desc, instr, buffer);
		};
		callback_table.log = [](CPU& cpu, address_t pc, const char* msg) {
			printf("[trace] PC=0x%lx (0x%lX) %s\n", (unsigned long)pc, (unsigned long)cpu.pc(), msg);
		};
		callback_table.fallback = [](CPU& cpu, address_t pc, uint32_t instr) {
			auto decoded = cpu.decode(la_instruction{instr});
			char buffer[256];
			decoded.printer(buffer, sizeof(buffer), cpu, la_instruction{instr}, pc);
			printf("[trace] PC=0x%lx: fallback 0x%08x: %s\n", (unsigned long)pc, instr, buffer);
		};
		callback_table.sqrtf32 = [](float x) { return __builtin_sqrtf(x); };
		callback_table.sqrtf64 = [](double x) { return __builtin_sqrt(x); };
		callback_table.clz = [](uint32_t x) { return __builtin_clz(x); };
		callback_table.clzl = [](uint64_t x) { return __builtin_clzl(x); };
		callback_table.ctz = [](uint32_t x) { return __builtin_ctz(x); };
		callback_table.ctzl = [](uint64_t x) { return __builtin_ctzl(x); };
		callback_table.cpop = [](uint32_t x) { return __builtin_popcount(x); };
		callback_table.cpopl = [](uint64_t x) { return __builtin_popcountl(x); };

		// Initialize the translated segment
		init_func(&callback_table, arena_info.arena_offset, arena_info.ic_offset);

		return true;
	}

	// Apply live-patching: atomically swap decoder cache entries
	static void apply_live_patch(const MachineOptions& options, DecodedExecuteSegment& exec,
		const Mapping* mappings, unsigned nmappings)
	{
		// Issue memory fence to ensure patched decoder is visible
		std::atomic_thread_fence(std::memory_order_seq_cst);

		// Apply livepatch bytecode to all translated locations
		// This will cause execution to swap to the patched decoder cache
		for (unsigned i = 0; i < nmappings; i++)
		{
			const auto addr = mappings[i].addr;

			if (exec.is_within(addr)) {
				auto& entry = *exec.pc_relative_decoder_cache(addr);
				/// XXX: Set the livepatch bytecode atomically
				/// NOTE: handler_idx=0 means binary translation livepatch
				entry.set_bytecode(LA64_BC_LIVEPATCH);
				entry.handler_idx = 0;
			}
		}

		if (options.verbose_loader) {
			printf("libloong: Live-patching applied to %u locations\n", nmappings);
		}
	}

	// Activate the dylib by mapping handlers to decoder cache
	// live_patch=true will apply to patched decoder cache and use livepatch bytecode
	void activate_dylib(MachineOptions options, DecodedExecuteSegment& exec, void* dylib,
		const ArenaInfo& arena_info, bool is_libtcc, bool live_patch = false)
	{
		// Map all the functions to instruction handlers
		const uint32_t* no_mappings = (const uint32_t*)dylib_lookup(dylib, "no_mappings", is_libtcc);
		const auto* mappings = (const Mapping*)dylib_lookup(dylib, "mappings", is_libtcc);
		const uint32_t* no_handlers = (const uint32_t*)dylib_lookup(dylib, "no_handlers", is_libtcc);
		const auto* handlers = (const bintr_block_func*)dylib_lookup(dylib, "unique_mappings", is_libtcc);

		if (no_mappings == nullptr || mappings == nullptr || *no_mappings > 500000UL) {
			dylib_close(dylib, is_libtcc);
			exec.set_libtcc(false);
			throw MachineException(INVALID_PROGRAM, "Invalid mappings in binary translation program");
		}
		if (*no_mappings == 0)
			return;

		if (!initialize_translated_segment(exec, dylib, arena_info, is_libtcc))
		{
#ifdef ENABLE_LIBTCC
			if (options.verbose_loader) {
				fprintf(stderr, "libloong: Could not find dylib init function\n");
			}
#else
			(void)options;
#endif
			if (dylib != nullptr) {
				dylib_close(dylib, is_libtcc);
			}
			exec.set_libtcc(false);
			return;
		}

		// Mark segment as binary translated
		exec.set_libtcc(is_libtcc);

		const unsigned nmappings = *no_mappings;
		const unsigned unique_mappings = *no_handlers;

		// Create N+1 mappings, where the last one is a catch-all for invalid mappings
		auto& exec_mappings = exec.create_mappings(unique_mappings + 1);
		std::copy(handlers, handlers + unique_mappings, exec_mappings.begin());
		exec.set_mapping(unique_mappings, [](CPU&, uint64_t, uint64_t, address_t) -> bintr_block_returns {
			throw MachineException(INVALID_PROGRAM, "Translation mapping outside execute area");
		});

		// Debug: print the function pointers we're using
		if (options.verbose_loader) {
			for (unsigned i = 0; i < unique_mappings; i++) {
				printf("  Handler[%u] = %p\n", i, (void*)handlers[i]);
			}
		}

		// Choose which decoder cache to apply mappings to
		DecoderData* target_decoder = live_patch
			? exec.patched_decoder_cache()
			: exec.decoder_cache();

		if (live_patch) {
			// For live-patching, we need to copy the entire decoder cache first
			const size_t cache_size = exec.decoder_cache_size();
			if (!exec.patched_decoder_cache()) {
				auto* patched = new DecoderData[cache_size];
				std::copy(exec.decoder_cache(), exec.decoder_cache() + cache_size, patched);
				exec.set_patched_decoder_cache(patched, cache_size);
				target_decoder = patched;
			}
		}
		// Make decoder PC-relative
		target_decoder -= exec.exec_begin() >> DecoderCache::SHIFT;

		// Apply mappings to the target decoder cache
		for (unsigned i = 0; i < nmappings; i++)
		{
			const unsigned mapping_index = mappings[i].mapping_index;
			const auto addr = mappings[i].addr;

			if (exec.is_within(addr)) {
				// Calculate the decoder entry for this address
				auto* entry = target_decoder + (addr >> DecoderCache::SHIFT);
				entry->set_bytecode(LA64_BC_TRANSLATOR);
				entry->instr = mapping_index;
				entry->handler_idx = 0xFF; // Invalid handler index
			} else if (options.verbose_loader) {
				fprintf(stderr, "libloong: Mapping address 0x%lx outside execute area\n",
					(unsigned long)addr);
			}
		}

		if (options.verbose_loader) {
			printf("libloong: Binary translation %s with %u mappings and %u handlers\n",
				live_patch ? "prepared for live-patching" : "activated",
				nmappings, unique_mappings);
		}

		// Store the dylib handle for cleanup
		if (live_patch) {
			// For live-patching, we'll apply the patch after this function returns
			// Don't store the dylib yet - caller will do it
		} else {
			// For synchronous translation, store the dylib immediately
			exec.set_bintr_dylib(dylib);
		}
	}

	// Try to compile and activate binary translation
	bool try_translate(const Machine& machine, const MachineOptions& options,
		std::shared_ptr<DecodedExecuteSegment>& exec)
	{
#ifdef LA_BINARY_TRANSLATION
		if (!options.translate_enabled)
			return false;

		TransOutput output;

		try {
			// Generate C code for binary translation
			binary_translate(machine, options, *exec, output);

			if (!output.code || output.code->empty()) {
				if (options.verbose_loader) {
					fprintf(stderr, "libloong: Binary translation produced no code\n");
				}
				return false;
			}
			if (output.mappings.empty()) {
				if (options.verbose_loader) {
					fprintf(stderr, "libloong: Binary translation produced no mappings\n");
				}
				return false;
			}

			// Append footer
			*output.code += output.footer;

#ifdef ENABLE_LIBTCC
			// Determine if we should use live-patching
			const bool use_live_patch = (options.translate_background_callback != nullptr);

			// Prepare arena information for background compilation
			ArenaInfo arena_info;
			auto* arena_ptr = machine.memory.arena_ref();
			arena_info.arena_ptr = (const uint8_t*)arena_ptr;
			arena_info.arena_offset = reinterpret_cast<intptr_t>((intptr_t)arena_ptr - (intptr_t)&machine);
			arena_info.ic_offset = Machine::counter_offset();

			// Create the compilation step as a lambda
			// Capture by value to ensure thread safety
			auto compilation_step = [options, exec_ptr = exec, output = std::move(output), use_live_patch, arena_info]() mutable -> void {
				// Protect libtcc_compile with a mutex
				static std::mutex libtcc_mutex;
				std::lock_guard<std::mutex> lock(libtcc_mutex);

				// Mark that we're compiling in the background
				if (use_live_patch) {
					exec_ptr->set_background_compiling(true);
				}

				try {
					// Compile with libtcc
					void* dylib = libtcc_compile(*output.code, {}, "");
					if (dylib == nullptr) {
						if (options.verbose_loader) {
							fprintf(stderr, "libloong: libtcc compilation failed\n");
						}
						if (use_live_patch) {
							exec_ptr->set_background_compiling(false);
						}
						return;
					}

					// Activate the compiled code
					activate_dylib(options, *exec_ptr, dylib, arena_info, true, use_live_patch);

					if (use_live_patch) {
						// Store the dylib handle
						exec_ptr->set_bintr_dylib(dylib);
						// Get mappings for live-patching
						const uint32_t* no_mappings = (const uint32_t*)dylib_lookup(dylib, "no_mappings", true);
						const auto* mappings = (const Mapping*)dylib_lookup(dylib, "mappings", true);

						if (no_mappings && mappings) {
							// Apply the live-patch
							apply_live_patch(options, *exec_ptr, mappings, *no_mappings);
						}

						// Mark compilation as complete
						exec_ptr->set_background_compiling(false);
					}
				} catch (const std::exception& e) {
					if (options.verbose_loader) {
						fprintf(stderr, "libloong: Binary translation compilation failed: %s\n", e.what());
					}
					if (use_live_patch) {
						exec_ptr->set_background_compiling(false);
					}
				}
			};

			// Execute the compilation step
			if (use_live_patch) {
				// Call the user-provided background callback
				options.translate_background_callback(compilation_step);
			} else {
				// Execute synchronously in the same thread
				compilation_step();
			}

			return true;
#else
			if (options.verbose_loader) {
				fprintf(stderr, "libloong: Binary translation enabled but libtcc not compiled in\n");
			}
			return false;
#endif
		} catch (const std::exception& e) {
			if (options.verbose_loader) {
				fprintf(stderr, "libloong: Binary translation failed: %s\n", e.what());
			}
			return false;
		}
#else
		(void)machine;
		(void)options;
		(void)exec;
		return false;
#endif
	}

} // namespace loongarch
