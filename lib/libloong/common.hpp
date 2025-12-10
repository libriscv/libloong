#pragma once
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <libloong_settings.h>

namespace loongarch
{
	#define LA_SYSCALLS_MAX  512

	struct MachineOptions {
		size_t memory_max = 256 * 1024 * 1024; // 256 MB default
		size_t stack_size = 2 * 1024 * 1024;   // 2 MB default stack
		size_t brk_size   = 1 * 1024 * 1024;   // 1 MB default brk area
		bool verbose_loader = false;
		bool ignore_text_section = false;
		bool verbose_syscalls = false;

#ifdef LA_BINARY_TRANSLATION
		// Binary translation options
		bool translate_enabled = true;
		bool translate_trace = false;
		bool translate_ignore_instruction_limit = false;
		bool use_shared_execute_segments = false;
		bool translate_use_register_caching = true;
		/// @brief Enable automatic n-bit address space for the binary translator by rounding down to the nearest power of 2.
		/// @details This will allow the binary translator to use and-masked addresses
		/// for all memory accesses, which can drastically improve performance.
		bool translate_automatic_nbit_address_space = true;
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
