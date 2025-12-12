#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <libloong_settings.h>

namespace loongarch
{
	#define LA_SYSCALLS_MAX  512
	#define LA_OVER_ALLOCATE_SIZE 64 /* Avoid SIMD bounds-check */

	struct MachineOptions {
		size_t memory_max = 256 * 1024 * 1024; // 256 MB default
		size_t stack_size = 2 * 1024 * 1024;   // 2 MB default stack
		size_t brk_size   = 1 * 1024 * 1024;   // 1 MB default brk area
		bool verbose_loader = false;
		bool ignore_text_section = false;
		bool verbose_syscalls = false;
		/// @brief Enable sharing of execute segments between machines.
		/// @details This will allow multiple machines to share the same execute
		/// segment, reducing memory usage and increasing performance.
		/// When binary translation is enabled, this will also share the dynamically
		/// translated code between machines. (Prevents some optimizations)
		bool use_shared_execute_segments = true;

		/// @brief Donate a custom arena for the machine to use.
		/// @details If this pointer is non-null, the machine will use the provided
		/// arena pointer and size instead of allocating its own. If the pointer is
		/// relative to the machine itself, the CPU* pointer avoids a
		/// double indirection for memory accesses in binary translation.
		/// The size should be over-allocated by LA_OVER_ALLOCATE_SIZE bytes to avoid
		/// SIMD bounds-check issues, although this does not have to be deducted by
		/// the user. Example: In order to make use of (uint32_t)-masked memory
		/// accesses, the arena size should be 4GB + LA_OVER_ALLOCATE_SIZE bytes.
		void* custom_arena_pointer = nullptr;
		size_t custom_arena_size = 0;
		/// @brief If using a custom arena, specify the CPU-relative offset
		/// @param memory_max The maximum size of the machines internal memory
		/// @return A pair of (total_arena_size, arena_offset / cpu_relative_offset)
		struct CustomArenaInfo {
			size_t total_size;
			size_t arena_offset;
			size_t arena_size;
		};
		static CustomArenaInfo estimate_cpu_relative_arena_size_for(size_t memory_max);

#ifdef LA_BINARY_TRANSLATION
		// Binary translation options
		bool translate_enabled = true;
		bool translate_trace = false;
		bool translate_ignore_instruction_limit = false;
		/// @brief Enable register caching in the binary translator.
		/// @details This will cache frequently used registers in real CPU registers,
		/// improving performance at the cost of higher entry/exit overheads.
		bool translate_use_register_caching = true;
		/// @brief A callback that is invoked to perform the binary translation and
		/// compilation step in the background.
		/// @details This callback will be called by the emulator when it
		/// a binary translation is ready to be compiled. The user may (should)
		/// call this callback in a separate thread to perform the compilation
		/// step without blocking the main emulation thread.
		std::function<void(const std::function<void()>& compilation_step)> translate_background_callback = nullptr;
		/// @brief Enable automatic n-bit address space for the binary translator by rounding down to the nearest power of 2.
		/// @details This will allow the binary translator to use and-masked addresses
		/// for all memory accesses, which can drastically improve performance.
		bool translate_automatic_nbit_address_space = false;
		/// @brief Enable unchecked memory accesses in the binary translator, which
		/// will cause hard faults on invalid accesses instead of raising exceptions.
		bool translate_unchecked_memory_accesses = false;
		/// @brief Verbose logging for binary translation fallbacks
		/// Observe which instructions do not have a binary translation emitted
		bool translate_verbose_fallbacks = false;
		size_t translate_blocks_max = 10000;
		size_t translate_instr_max = 50'000'000ull;
		std::string translate_output_file; // Optional: output file path for generated C code
#endif
	};

	// Address types for 64-bit LoongArch
	using address_t = uint64_t;
	using saddress_t = int64_t;

	// Forward declarations
	struct Machine;
	struct CPU;
	struct Memory;
	struct Registers;
	struct DecodedExecuteSegment;
	struct DecoderCache;
	union la_instruction;

	// Machine exceptions
	enum ExceptionType {
		ILLEGAL_OPCODE,
		ILLEGAL_OPERATION,
		PROTECTION_FAULT,
		EXECUTION_SPACE_PROTECTION_FAULT,
		MISALIGNED_INSTRUCTION,
		UNIMPLEMENTED_INSTRUCTION,
		MACHINE_TIMEOUT,
		OUT_OF_MEMORY,
		INVALID_PROGRAM,
		FEATURE_DISABLED,
		UNIMPLEMENTED_SYSCALL,
		GUEST_ABORT,
	};

	class MachineException : public std::runtime_error {
	public:
		MachineException(ExceptionType type, const char* msg, uint64_t data = 0)
			: std::runtime_error(msg), m_type(type), m_data(data) {}

		ExceptionType type() const noexcept { return m_type; }
		uint64_t data() const noexcept { return m_data; }

	private:
		ExceptionType m_type;
		uint64_t m_data;
	};

	class MachineTimeoutException : public MachineException {
	public:
		MachineTimeoutException()
			: MachineException(MACHINE_TIMEOUT, "Machine instruction timeout") {}
	};

	struct Arena;

	// Compiler attributes
	#if defined(__GNUC__) || defined(__clang__)
		#define LA_INLINE      __attribute__((always_inline)) inline
		#define LA_NOINLINE    __attribute__((noinline))
		#define LA_COLD_PATH() __attribute__((cold))
		#define LA_HOT_PATH()  __attribute__((hot))
		#define LA_LIKELY(x)   __builtin_expect(!!(x), 1)
		#define LA_UNLIKELY(x) __builtin_expect(!!(x), 0)
	#else
		#define LA_INLINE      inline
		#define LA_NOINLINE
		#define LA_COLD_PATH()
		#define LA_HOT_PATH()
		#define LA_LIKELY(x)   (x)
		#define LA_UNLIKELY(x) (x)
	#endif

	#define LA_ALWAYS_INLINE LA_INLINE

	template <typename T>
	using remove_cvref_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

	template <class...>
	constexpr std::false_type always_false {};

	template <typename T>
	struct is_stdstring : public std::is_same<T, std::basic_string<char>> {};
} // namespace loongarch
