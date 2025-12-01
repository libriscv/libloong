#pragma once
#include "common.hpp"
#include "cpu.hpp"
#include "memory.hpp"
#include <string>
#include <vector>
#include <functional>

namespace loongarch
{
	struct alignas(LA_MACHINE_ALIGNMENT) Machine
	{
		using syscall_t = void(Machine&);

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
		void system_call(unsigned sysnum);
		void unchecked_system_call(unsigned sysnum);
		template <typename T = address_t>
		void set_result(const T& value);
		template <typename T = address_t>
		T return_value() const;

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
		template <uint64_t MAX_INSTRUCTIONS = UINT64_MAX, typename... Args>
		address_t vmcall(address_t func_addr, Args&&... args);

		template <uint64_t MAX_INSTRUCTIONS = UINT64_MAX, typename... Args>
		address_t vmcall(const std::string& func_name, Args&&... args);

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

		// Components
		CPU cpu;
		Memory memory;

		// Multi-threading support
		struct ThreadData {
			int tid = 1;
			address_t clear_tid_addr = 0;
			address_t robust_list = 0;
		};
		int gettid() const noexcept { return m_threads.tid; }
		void set_tid_address(address_t addr) noexcept { m_threads.clear_tid_addr = addr; }
		address_t get_tid_address() const noexcept { return m_threads.clear_tid_addr; }

		// Options
		bool has_options() const noexcept { return m_options_ptr != nullptr; }
		const MachineOptions& options() const { return *m_options_ptr; }

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

	private:
		uint64_t      m_counter = 0;
		uint64_t      m_max_instructions = 0;
		mutable void* m_userdata = nullptr;
		const MachineOptions* m_options_ptr = nullptr;
		ThreadData    m_threads;
		std::unique_ptr<Arena> m_arena;
		static inline std::array<syscall_t*, LA_SYSCALLS_MAX> m_syscall_handlers = {};

		void push_argument(address_t& sp, address_t value);

		// Helper for sysargs
		template<typename... Args, std::size_t... Indices>
		inline auto resolve_args(std::index_sequence<Indices...>) const;
	};

} // namespace loongarch

#include "machine_inline.hpp"
#include "machine_vmcall.hpp"
