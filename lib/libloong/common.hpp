#pragma once
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <libloong_settings.h>

namespace loongarch
{
	// LoongArch address widths
	static constexpr int LA32 = 4;
	static constexpr int LA64 = 8;
	#define LA_SYSCALLS_MAX  512

	template <int W = LA64>
	struct MachineOptions {
		size_t memory_max = 256 * 1024 * 1024; // 256 MB default
		size_t stack_size = 2 * 1024 * 1024;   // 2 MB default stack
		size_t brk_size   = 1 * 1024 * 1024;   // 1 MB default brk area
		bool verbose_loader = false;
		bool ignore_text_section = false;
		bool verbose_syscalls = false;
	};

	// Address types
	template <int W>
	struct address_type_helper {
		using type = uint32_t;
	};
	template <>
	struct address_type_helper<8> {
		using type = uint64_t;
	};
	template <int W>
	using address_type = typename address_type_helper<W>::type;

	template <int W>
	using signed_address_type = typename std::make_signed<address_type<W>>::type;

	// Forward declarations
	template <int W> struct Machine;
	template <int W> struct CPU;
	template <int W> struct Memory;
	template <int W> struct Registers;
	template <int W> struct DecodedExecuteSegment;
	template <int W> struct DecoderCache;
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
