#pragma once
#include "common.hpp"
#include "cpu.hpp"
#include "memory.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace loongarch
{
	// Forward declarations
	struct Signals;
	struct SignalAction;
	struct MultiThreading;

	struct alignas(LA_MACHINE_ALIGNMENT) Machine
	{
		using syscall_t = void(Machine&);
		using unknown_syscall_t = void(Machine&, int);
		using rdtime_callback_t = uint64_t(Machine&);

		// Construction
		Machine(std::string_view binary, const MachineOptions& options = {});
		Machine(const std::vector<uint8_t>& binary, const MachineOptions& options = {});
		Machine(const Machine&) = delete;
		~Machine();

		/// @brief Set a custom pointer that only you know the meaning of.
		/// This pointer can be retrieved from many of the callbacks in the
		/// machine, such as system calls, printers etc. It is used to
		/// facilitate wrapping the RISC-V Machine inside of your custom
		/// structure, such as a Script class.
		/// @tparam T The type of the pointer.
		/// @param data The pointer to the outer class.
		template <typename T> void set_userdata(T* data) { m_userdata = data; }

		/// @brief Return a previously set user pointer. It is usually
		/// a pointer to an outer wrapper class that manages the Machine, such
		/// as a Script class.
		/// @tparam T The type of the previously set user pointer.
		/// @return The previously set user pointer.
		template <typename T> T* get_userdata() const noexcept { return static_cast<T*> (m_userdata); }

		// Setup
		void setup_linux(
			const std::vector<std::string>& args,
			const std::vector<std::string>& env);
		static void setup_minimal_syscalls();
		static void setup_linux_syscalls();
		void setup_accelerated_syscalls(); // Warning: modifies decoder cache
		void set_options(const std::shared_ptr<MachineOptions> options); // Non-owning reference

		// Execution
		bool simulate(uint64_t max_instructions = UINT64_MAX, uint64_t counter = 0);

		void stop() noexcept { m_max_instructions = 0; }
		bool stopped() const noexcept { return m_counter >= m_max_instructions; }
		bool instruction_limit_reached() const noexcept { return m_counter >= m_max_instructions && m_max_instructions != 0; }

		// Instruction counting
		uint64_t instruction_counter() const noexcept { return m_counter; }
		void set_instruction_counter(uint64_t val) noexcept { m_counter = val; }
		void increment_counter(uint64_t val) noexcept { m_counter += val; }

		uint64_t max_instructions() const noexcept { return m_max_instructions; }
		void set_max_instructions(uint64_t val) noexcept { m_max_instructions = val; }

		// System call interface
		static void install_syscall_handler(unsigned sysnum, syscall_t* handler);
		static void set_unknown_syscall_handler(unknown_syscall_t* handler);
		static syscall_t** get_syscall_handlers();
		static unknown_syscall_t* get_unknown_syscall_handler();
		void system_call(unsigned sysnum);
		void unchecked_system_call(unsigned sysnum);
		template <typename T = address_t>
		void set_result(const T& value);
		template <typename T = address_t>
		T return_value() const;

		// rdtime.d callback interface
		static void set_rdtime(rdtime_callback_t* callback);
		static rdtime_callback_t* get_rdtime_handler() { return m_rdtime_handler; }
		uint64_t rdtime();

		// System call argument helpers
		template <typename T = address_t>
		inline T sysarg(int idx) const;

		/// @brief Retrieve a tuple of arguments based on the given types.
		/// Example: auto [str, i, f] = machine.sysargs<std::string, int, float>();
		/// Example: auto [addr, len] = machine.sysargs<address_t, unsigned>();
		/// Note: String views consume 2 registers each (address and length).
		/// Example: auto [view] = machine.sysargs<std::string_view>();
		/// @tparam ...Args A list of argument types.
		/// @return The resolved arguments in a tuple.
		template <typename... Args>
		inline auto sysargs() const;

		// Function calls (implemented in machine_vmcall.hpp)
		template <typename Ret = address_t, uint64_t MAX_INSTRUCTIONS = UINT64_MAX, typename... Args>
		Ret vmcall(address_t func_addr, Args&&... args);

		template <typename Ret = address_t, uint64_t MAX_INSTRUCTIONS = UINT64_MAX, typename... Args>
		Ret vmcall(const std::string& func_name, Args&&... args);

		// Timed function call with run-time instruction limit
		// Use Machine::return_value<T>() to get typed return values
		template <typename... Args>
		void timed_vmcall(address_t func_addr, uint64_t max_instructions, Args&&... args);

		// Preemptible function calls with instruction limit
		template <bool Throw = true, bool StoreRegs = false, typename... Args>
		address_t preempt(uint64_t max_instr, address_t func_addr, Args&&... args);

		template <bool Throw = true, bool StoreRegs = false, typename... Args>
		address_t preempt(uint64_t max_instr, const std::string& func_name, Args&&... args);

		// Stack manipulation helpers (for vmcall)
		address_t stack_push(address_t& sp, const void* data, size_t size);

		template <typename T>
		address_t stack_push(address_t& sp, const T& value);

		// Symbol lookup (delegates to memory)
		address_t address_of(const std::string& name) const;
		const Symbol* lookup_symbol(address_t addr) const;
		std::string lookup_demangled_symbol(address_t addr, bool with_offset = true) const;
		std::string backtrace(address_t initial = 0) const;

		// Components
		CPU cpu;
		Memory memory;

		// Options
		bool has_options() const noexcept { return m_options != nullptr; }
		const MachineOptions& options() const { return *m_options; }

		// Optional custom native-performance heap
		bool has_arena() const noexcept { return m_arena != nullptr; }
		const Arena& arena() const;
		Arena& arena();
		void setup_accelerated_heap(address_t arena_base, size_t arena_size); // Creates arena if needed

		// Serialization
		size_t serialize_to(std::vector<uint8_t>& vec) const;
		int deserialize_from(const std::vector<uint8_t>& vec);

		// Print helper
		void print(const char* data, size_t len);
		void print(std::string_view str);

		// Bytecode statistics
		struct BytecodeStats {
			uint8_t bytecode;
			uint64_t count;
			uint32_t sample_instruction; // Sample instruction bits for fallback bytecodes
		};
		std::vector<BytecodeStats> collect_bytecode_statistics() const;
		bool is_binary_translation_enabled() const noexcept;

		// Signal handling
		Signals& signals();
		SignalAction& sigaction(int sig);

		// Threading support
		bool has_threads() const noexcept { return m_mt != nullptr; }
		MultiThreading& threads();
		int gettid();
		void setup_posix_threads();
		static intptr_t counter_offset() noexcept;

		// Current machine exception (used to avoid unwinding)
		void set_current_exception(std::exception_ptr&& ptr) noexcept { m_current_exception = std::move(ptr); }
		void clear_current_exception() noexcept { m_current_exception = nullptr; }
		bool has_current_exception() const noexcept { return m_current_exception != nullptr; }
		auto& current_exception() const noexcept { return m_current_exception; }

	private:
		uint64_t      m_counter = 0;
		uint64_t      m_max_instructions = 0;
		mutable void* m_userdata = nullptr;
		std::shared_ptr<MachineOptions> m_options = nullptr;
		std::unique_ptr<Arena> m_arena;
		std::unique_ptr<Signals> m_signals;
		std::unique_ptr<MultiThreading> m_mt;
		std::exception_ptr m_current_exception = nullptr;
		static inline std::array<syscall_t*, LA_SYSCALLS_MAX> m_syscall_handlers = {};
		static inline unknown_syscall_t* m_unknown_syscall_handler = nullptr;
		static inline rdtime_callback_t* m_rdtime_handler = nullptr;

		void push_argument(address_t& sp, address_t value);

		// Helper for sysargs
		template<typename... Args, std::size_t... Indices>
		inline auto resolve_args(std::index_sequence<Indices...>) const;
	};

} // namespace loongarch

#include "machine_inline.hpp"
#include "machine_vmcall.hpp"
