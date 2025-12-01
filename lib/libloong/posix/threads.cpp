#include "threads.hpp"
#include "../registers.hpp"

namespace loongarch
{
	// Error numbers
	static constexpr int64_t LA_ENOSYS = 38;
	static constexpr int64_t LA_EINVAL = 22;
	static constexpr int64_t LA_EAGAIN = 11;
	static constexpr int64_t LA_ESRCH = 3;

	// Clone flags (prefixed to avoid conflicts with system macros)
	static constexpr int LA_CLONE_VM = 0x00000100;
	static constexpr int LA_CLONE_FS = 0x00000200;
	static constexpr int LA_CLONE_FILES = 0x00000400;
	static constexpr int LA_CLONE_SIGHAND = 0x00000800;
	static constexpr int LA_CLONE_THREAD = 0x00010000;
	static constexpr int LA_CLONE_SYSVSEM = 0x00040000;
	static constexpr int LA_CLONE_SETTLS = 0x00080000;
	static constexpr int LA_CLONE_PARENT_SETTID = 0x00100000;
	static constexpr int LA_CLONE_CHILD_CLEARTID = 0x00200000;
	static constexpr int LA_CLONE_CHILD_SETTID = 0x01000000;

	// LoongArch syscall numbers
	static constexpr int LA_SYS_clone = 220;
	static constexpr int LA_SYS_clone3 = 435;
	static constexpr int LA_SYS_exit = 93;
	static constexpr int LA_SYS_exit_group = 94;
	static constexpr int LA_SYS_futex = 98;
	static constexpr int LA_SYS_set_tid_address = 96;
	static constexpr int LA_SYS_gettid = 178;
	static constexpr int LA_SYS_tgkill = 131;
	static constexpr int LA_SYS_tkill = 130;

	// Clone syscall
	// LoongArch ABI: clone(flags, stack, parent_tid, tls, child_tid)
	static void syscall_clone(Machine& machine)
	{
		auto flags = machine.cpu.reg(REG_A0);
		auto stack = machine.cpu.reg(REG_A1);
		auto parent_tidptr = machine.cpu.reg(REG_A2);
		auto tls = machine.cpu.reg(REG_A3);
		auto child_tidptr = machine.cpu.reg(REG_A4);

		(void)stack;
		(void)parent_tidptr;
		(void)child_tidptr;

		// Check if this is a thread creation
		const int thread_flags = LA_CLONE_VM | LA_CLONE_FS | LA_CLONE_FILES | LA_CLONE_SIGHAND | LA_CLONE_THREAD;
		if ((flags & thread_flags) == thread_flags) {
			// Thread creation - not supported in single-threaded mode
			// Return EAGAIN to indicate resource temporarily unavailable
			machine.set_result(-LA_EAGAIN);
			return;
		}

		// Check for TLS setup
		if (flags & LA_CLONE_SETTLS) {
			// Set thread pointer register to the provided TLS address
			machine.cpu.reg(REG_TP) = tls;
		}

		// For non-thread clone (fork), return ENOSYS
		machine.set_result(-LA_ENOSYS);
		return;
	}

	// Clone3 syscall (extended clone interface)
	static void syscall_clone3(Machine& machine)
	{
		auto args_addr = machine.cpu.reg(REG_A0);
		auto size = machine.cpu.reg(REG_A1);

		// Clone3 args structure (partial, common fields)
		struct clone3_args {
			uint64_t flags;
			uint64_t pidfd;
			uint64_t child_tid;
			uint64_t parent_tid;
			uint64_t exit_signal;
			uint64_t stack;
			uint64_t stack_size;
			uint64_t tls;
		};

		if (size < sizeof(clone3_args)) {
			machine.set_result(-LA_EINVAL);
			return;
		}

		// Read flags and TLS from args structure
		auto flags = machine.memory.template read<uint64_t>(args_addr);
		auto tls = machine.memory.template read<uint64_t>(args_addr + 56);  // offset of tls

		// Check if this is a thread creation
		const int thread_flags = LA_CLONE_VM | LA_CLONE_FS | LA_CLONE_FILES | LA_CLONE_SIGHAND | LA_CLONE_THREAD;
		if ((flags & thread_flags) == thread_flags) {
			// Thread creation - not supported
			machine.set_result(-LA_EAGAIN);
			return;
		}

		// TLS setup
		if (flags & LA_CLONE_SETTLS) {
			machine.cpu.reg(REG_TP) = tls;
		}

		machine.set_result(-LA_ENOSYS);
		return;
	}

	// Set TID address syscall
	static void syscall_set_tid_address(Machine& machine)
	{
		auto tidptr = machine.cpu.reg(REG_A0);
		machine.set_tid_address(tidptr);
		machine.set_result(machine.gettid());
	}

	// Gettid syscall
	static void syscall_gettid(Machine& machine)
	{
		machine.set_result(machine.gettid());
	}

	// Exit syscall with thread cleanup
	static void syscall_exit(Machine& machine)
	{
		// Clear TID at the registered address (CLONE_CHILD_CLEARTID)
		auto clear_addr = machine.get_tid_address();
		if (clear_addr != 0) {
			machine.memory.template write<uint32_t>(clear_addr, 0);
			// In a multi-threaded implementation, we would wake futex waiters here
		}

		machine.stop();
	}

	// Futex syscall
	static void syscall_futex(Machine& machine)
	{
		auto uaddr = machine.cpu.reg(REG_A0);
		int futex_op = machine.cpu.reg(REG_A1);
		auto val = machine.cpu.reg(REG_A2);
		// auto timeout = machine.cpu.reg(REG_A3);
		// auto uaddr2 = machine.cpu.reg(REG_A4);
		// auto val3 = machine.cpu.reg(REG_A5);

		constexpr int FUTEX_WAIT = 0;
		constexpr int FUTEX_WAKE = 1;
		constexpr int FUTEX_REQUEUE = 3;
		constexpr int FUTEX_CMP_REQUEUE = 4;
		constexpr int FUTEX_WAKE_OP = 5;
		constexpr int FUTEX_WAIT_BITSET = 9;
		constexpr int FUTEX_WAKE_BITSET = 10;
		constexpr int FUTEX_PRIVATE_FLAG = 128;

		int op = futex_op & ~FUTEX_PRIVATE_FLAG;

		switch (op) {
			case FUTEX_WAIT:
			case FUTEX_WAIT_BITSET: {
				// Check if the value at uaddr matches val
				auto current = machine.memory.template read<uint32_t>(uaddr);
				if (current != static_cast<uint32_t>(val)) {
					// Value changed, return EAGAIN
					machine.set_result(-LA_EAGAIN);
					return;
				}
				// In single-threaded mode, we can't actually wait
				// Return EAGAIN to prevent deadlock
				machine.set_result(-LA_EAGAIN);
				return;
			}
			case FUTEX_WAKE:
			case FUTEX_WAKE_BITSET:
				// No threads to wake in single-threaded mode
				machine.set_result(0);
				return;
			case FUTEX_REQUEUE:
			case FUTEX_CMP_REQUEUE:
			case FUTEX_WAKE_OP:
				// Not supported in single-threaded mode
				machine.set_result(0);
				return;
			default:
				machine.set_result(-LA_ENOSYS);
				return;
		}
	}

	// Tgkill syscall
	static void syscall_tgkill(Machine& machine)
	{
		// int tgid = machine.cpu.reg(REG_A0);
		int tid = machine.cpu.reg(REG_A1);
		// int sig = machine.cpu.reg(REG_A2);

		// In single-threaded mode, only TID 1 exists
		if (tid != machine.gettid()) {
			machine.set_result(-LA_ESRCH);
			return;
		}
		// Signal handling not implemented
		machine.set_result(0);
	}

	// Tkill syscall
	static void syscall_tkill(Machine& machine)
	{
		int tid = machine.cpu.reg(REG_A0);
		// int sig = machine.cpu.reg(REG_A1);

		if (tid != machine.gettid()) {
			machine.set_result(-LA_ESRCH);
			return;
		}
		machine.set_result(0);
	}

	void setup_posix_threads(Machine& machine)
	{
		// Thread creation
		machine.install_syscall_handler(LA_SYS_clone, syscall_clone);
		machine.install_syscall_handler(LA_SYS_clone3, syscall_clone3);

		// Thread identification
		machine.install_syscall_handler(LA_SYS_set_tid_address, syscall_set_tid_address);
		machine.install_syscall_handler(LA_SYS_gettid, syscall_gettid);

		// Thread synchronization
		machine.install_syscall_handler(LA_SYS_futex, syscall_futex);

		// Thread termination
		machine.install_syscall_handler(LA_SYS_exit, syscall_exit);
		machine.install_syscall_handler(LA_SYS_exit_group, syscall_exit);

		// Thread signals
		machine.install_syscall_handler(LA_SYS_tgkill, syscall_tgkill);
		machine.install_syscall_handler(LA_SYS_tkill, syscall_tkill);
	}

} // loongarch
