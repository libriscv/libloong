#include "../machine.hpp"
#include "../posix/threads.hpp"
#include "../posix/signals.hpp"
#include <cstdio>

namespace loongarch
{
	// Error numbers
	static constexpr int64_t LA_EINVAL = 22;
	static constexpr int64_t LA_EAGAIN = 11;
	static constexpr int64_t LA_ENOSPC = 28;

	// Syscall numbers (LoongArch Linux ABI)
	enum LA_ThreadSyscalls {
		LA_SYS_futex_time64 = 422,
		LA_SYS_sched_yield = 124,
	};

	template <typename... Args>
	static inline void thprint(Machine& machine, const char* fmt, Args... args)
	{
		if (!machine.has_options() || !machine.options().verbose_syscalls) {
			return;
		}
		char buffer[512];
		int len = snprintf(buffer, sizeof(buffer), fmt, args...);
		if (len > 0) {
			machine.print(buffer, static_cast<size_t>(len));
		}
	}

	static inline void futex_op(Machine& machine,
		address_t addr, int futex_op, int val, uint32_t val3)
	{
		constexpr int FUTEX_WAIT = 0;
		constexpr int FUTEX_WAKE = 1;
		constexpr int FUTEX_WAIT_BITSET = 9;
		constexpr int FUTEX_WAKE_BITSET = 10;

		thprint(machine, ">>> futex(0x%llx, op=%d (0x%X), val=%d val3=0x%X)\n",
			static_cast<unsigned long long>(addr), futex_op & 0xF, futex_op, val, val3);

		if ((futex_op & 0xF) == FUTEX_WAIT || (futex_op & 0xF) == FUTEX_WAIT_BITSET)
		{
			const bool is_bitset = (futex_op & 0xF) == FUTEX_WAIT_BITSET;
			if (machine.memory.read<uint32_t>(addr) == static_cast<uint32_t>(val)) {
				thprint(machine,
					"FUTEX: Waiting (blocked)... uaddr=0x%llx val=%d, bitset=%d\n",
					static_cast<unsigned long long>(addr), val, is_bitset);
				if (machine.threads().block(0, addr, is_bitset ? val3 : 0x0)) {
					return;
				}
				// Deadlock - unlock and continue
				machine.memory.write<address_t>(addr, 0);
				machine.set_result(0);
				return;
			}
			thprint(machine,
				"FUTEX: Wait condition EAGAIN... uaddr=0x%llx val=%d, bitset=%d\n",
				static_cast<unsigned long long>(addr), val, is_bitset);
			// This thread isn't blocked, but yielding may be necessary
			machine.threads().suspend_and_yield(-LA_EAGAIN);
			return;
		} else if ((futex_op & 0xF) == FUTEX_WAKE || (futex_op & 0xF) == FUTEX_WAKE_BITSET) {
			const bool is_bitset = (futex_op & 0xF) == FUTEX_WAKE_BITSET;
			thprint(machine,
				"FUTEX: Waking %d others on 0x%llx, bitset=%d\n", val,
				static_cast<unsigned long long>(addr), is_bitset);
			unsigned awakened = machine.threads().wakeup_blocked(val, addr, is_bitset ? val3 : ~0x0);
			machine.set_result(awakened);
			thprint(machine, "FUTEX: Awakened: %u\n", awakened);
			return;
		}
		thprint(machine, "WARNING: Unhandled futex op: %X\n", futex_op);
		machine.set_result(-LA_EINVAL);
	}

	void Machine::setup_posix_threads()
	{
		if (!this->m_mt)
			this->m_mt = std::make_unique<MultiThreading>(*this);

		// exit & exit_group
		this->install_syscall_handler(93,
		[](Machine& machine) {
			[[maybe_unused]] const int status = machine.sysarg<int>(0);
			thprint(machine,
				">>> Exit on tid=%d, exit code = %d\n",
					machine.threads().get_tid(), static_cast<int>(status));
			// Exit returns true if the program ended
			if (!machine.threads().get_thread()->exit()) {
				// Should be a new thread now
				return;
			}
			machine.stop();
		});
		// exit_group
		this->install_syscall_handler(94, m_syscall_handlers[93]);

		// set_tid_address
		this->install_syscall_handler(96,
		[](Machine& machine) {
			const int clear_tid = machine.sysarg<address_t>(0);
			// With initialized threads
			if (machine.has_threads()) {
				machine.threads().get_thread()->clear_tid = clear_tid;
				machine.set_result(machine.threads().get_tid());
			} else {
				machine.set_result(0);
			}
			thprint(machine,
				">>> set_tid_address(0x%llx) = %d\n",
				static_cast<unsigned long long>(clear_tid),
				machine.return_value<int>());
		});

		// set_robust_list
		this->install_syscall_handler(99,
		[](Machine& machine) {
			address_t addr = machine.sysarg<address_t>(0);
			thprint(machine,
				">>> set_robust_list(0x%llx) = 0\n",
				static_cast<unsigned long long>(addr));
			(void)addr;
			machine.set_result(0);
		});

		// sched_yield
		this->install_syscall_handler(124,
		[](Machine& machine) {
			thprint(machine, "%s\n", ">>> sched_yield()");
			machine.threads().suspend_and_yield();
		});

		// tgkill (update to work with threads)
		this->install_syscall_handler(131,
		[](Machine& machine) {
			const int tid = machine.sysarg<int>(1);
			const int sig = machine.sysarg<int>(2);
			thprint(machine,
				">>> tgkill on tid=%d signal=%d\n", tid, sig);
			auto* thread = machine.threads().get_thread(tid);
			if (thread != nullptr) {
				// If the signal is unhandled, exit the thread
				if (sig != 0 && machine.sigaction(sig).is_unset()) {
					if (!thread->exit())
						return;
				} else {
					// Jump to signal handler and change to altstack, if set
					machine.signals().enter(machine, sig);
					thprint(machine,
						"<<< tgkill signal=%d jumping to 0x%llx (sp=0x%llx)\n",
						sig,
						static_cast<unsigned long long>(machine.sigaction(sig).handler),
						static_cast<unsigned long long>(machine.cpu.reg(REG_SP)));
					return;
				}
			}
			machine.stop();
		});

		// gettid
		this->install_syscall_handler(178,
		[](Machine& machine) {
			thprint(machine,
				">>> gettid() = %d\n", machine.threads().get_tid());
			machine.set_result(machine.threads().get_tid());
		});

		// futex
		this->install_syscall_handler(98,
		[](Machine& machine) {
			const auto addr = machine.sysarg<address_t>(0);
			const int fx_op = machine.sysarg<int>(1);
			const int   val = machine.sysarg<int>(2);
			const uint32_t val3 = machine.sysarg<uint32_t>(5);

			futex_op(machine, addr, fx_op, val, val3);
		});

		// futex_time64
		this->install_syscall_handler(422,
		[](Machine& machine) {
			const auto addr = machine.sysarg<address_t>(0);
			const int fx_op = machine.sysarg<int>(1);
			const int   val = machine.sysarg<int>(2);
			const uint32_t val3 = machine.sysarg<uint32_t>(5);

			futex_op(machine, addr, fx_op, val, val3);
		});

		// clone
		this->install_syscall_handler(220,
		[](Machine& machine) {
			/* int clone(int (*fn)(void *arg), int flags, void *child_stack, void *arg,
						 void *parent_tidptr, void *tls, void *child_tidptr) */
			const int  flags = machine.sysarg<int>(0);
			const auto stack = machine.sysarg<address_t>(1);
			const auto  ptid = machine.sysarg<address_t>(2);
			const auto  ctid = machine.sysarg<address_t>(3);
			auto         tls = machine.sysarg<address_t>(4);
			if (tls == 0x0) {
				tls = machine.cpu.reg(REG_TP);
			}
			auto* parent = machine.threads().get_thread();
			auto* thread = machine.threads().create(flags, ctid, ptid, stack, tls, 0, 0);
			thprint(machine,
				">>> clone(parent=%d, stack=0x%lx, flags=%x,"
					" ctid=0x%lx ptid=0x%lx, tls=0x%lx) = %d\n",
					parent->tid, stack, flags,
					ctid, ptid, tls, thread->tid);
			// store return value for parent: child TID
			parent->suspend(thread->tid);
			// activate and return 0 for the child
			thread->activate();
			machine.set_result(0);
		});

		// clone3
		this->install_syscall_handler(435,
		[](Machine& machine) {
			/* int clone3(struct clone3_args*, size_t len) */
			static constexpr uint32_t SETTLS = 0x00080000;
			struct clone3_args {
				address_t flags;
				address_t pidfd;
				address_t child_tid;
				address_t parent_tid;
				address_t exit_signal;
				address_t stack;
				address_t stack_size;
				address_t tls;
				address_t set_tid_array;
				address_t set_tid_count;
				address_t cgroup;
			};
			const auto [args, size] = machine.sysargs<clone3_args, address_t>();
			if (size < sizeof(clone3_args)) {
				machine.set_result(-LA_ENOSPC);
				return;
			}

			const int  flags = args.flags;
			const auto stack = args.stack + args.stack_size;
			const auto  ptid = args.parent_tid;
			const auto  ctid = args.child_tid;
			auto tls = args.tls;
			if ((args.flags & SETTLS) == 0) {
				tls = machine.cpu.reg(REG_TP);
			}

			auto* parent = machine.threads().get_thread();
			thprint(machine,
				">>> clone3(stack=0x%lx, flags=%x,"
					" parent=%d, ctid=0x%lx ptid=0x%lx, tls=0x%lx)\n",
					stack, flags, parent->tid, ctid, ptid, tls);
			auto* thread = machine.threads().create(flags, ctid, ptid, stack, tls, 0, 0);

			if (args.set_tid_count > 0) {
				address_t set_tid = 0;
				machine.memory.copy_from_guest(&set_tid, args.set_tid_array, sizeof(set_tid));
				thread->clear_tid = set_tid;
			}

			// store return value for parent: child TID
			parent->suspend(thread->tid);
			// activate and return 0 for the child
			thread->activate();
			machine.set_result(0);
		});
	}

} // namespace loongarch
