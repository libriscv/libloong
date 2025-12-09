#include "../machine.hpp"
#include "../posix/signals.hpp"
#include <cstring>
#include <cstdio>
#include <sys/time.h>
#include <unistd.h>

namespace loongarch
{
	// Error numbers (prefixed to avoid conflicts with system macros)
	static constexpr int64_t LA_ENOSYS = 38;
	static constexpr int64_t LA_ENOENT = 2;
	static constexpr int64_t LA_EBADF = 9;
	static constexpr int64_t LA_EINVAL = 22;
	static constexpr int64_t LA_EAGAIN = 11;
	static constexpr int64_t LA_ENOTTY = 25;

	// Syscall numbers (LoongArch Linux ABI)
	enum [[maybe_unused]] LA_Syscalls {
		LA_SYS_ioctl = 29,
		LA_SYS_fcntl = 25,
		LA_SYS_writev = 66,
		LA_SYS_exit = 93,
		LA_SYS_exit_group = 94,
		LA_SYS_set_tid_address = 96,
		LA_SYS_set_robust_list = 99,
		LA_SYS_futex = 98,
		LA_SYS_read = 63,
		LA_SYS_write = 64,
		LA_SYS_openat = 56,
		LA_SYS_close = 57,
		LA_SYS_ppoll = 73,
		LA_SYS_fstat = 80,
		LA_SYS_nanosleep = 101,
		LA_SYS_sched_getaffinity = 123,
		LA_SYS_gettimeofday = 169,
		LA_SYS_brk = 214,
		LA_SYS_mmap = 222,
		LA_SYS_mprotect = 226,
		LA_SYS_munmap = 215,
		LA_SYS_prlimit64 = 261,
		LA_SYS_readlinkat = 78,
		LA_SYS_getrandom = 278,
		LA_SYS_clock_gettime = 113,
		LA_SYS_gettid = 178,
		LA_SYS_getpid = 172,
		LA_SYS_getuid = 174,
		LA_SYS_geteuid = 175,
		LA_SYS_getgid = 176,
		LA_SYS_getegid = 177,
		LA_SYS_tkill = 130,
		LA_SYS_tgkill = 131,
		LA_SYS_sigaltstack = 132,
		LA_SYS_rt_sigaction = 134,
		LA_SYS_rt_sigprocmask = 135,
		LA_SYS_madvise = 233,
		LA_SYS_prctl = 167,
		LA_SYS_fstatat = 291,
	};

	template <typename... Args>
	static inline void sysprint(Machine& machine, const char* fmt, Args... args)
	{
		if (!machine.has_options() || !machine.options().verbose_syscalls) {
			return;
		}
		std::string buffer(512, '\0');
		int len = snprintf(buffer.data(), buffer.size(), fmt, args...);
		if (len > 0) {
			auto res = write(STDOUT_FILENO, buffer.data(), static_cast<size_t>(len));
			(void)res;
		}
	}

	// Exit syscall
	static void syscall_exit(Machine& machine)
	{
		machine.stop();
		sysprint(machine, "exit(status=%d)\n",
			machine.template return_value<int>());
	}

	// Write syscall
	static void syscall_write(Machine& machine)
	{
		auto [fd, addr, len] =
			machine.template sysargs<int, address_t, size_t>();

		if (fd == 1 || fd == 2) { // stdout or stderr
			const char* view = machine.memory.template memarray<char>(addr, len);
			machine.print(view, len);
			machine.set_result(len);
		} else {
			machine.set_result(-1);
		}
		sysprint(machine, "write(fd=%d, buf=0x%llx, count=%llu) = %d\n",
			fd,
			static_cast<uint64_t>(addr),
			static_cast<uint64_t>(len),
			machine.template return_value<int>());
	}

	// Writev syscall (write multiple buffers)
	static void syscall_writev(Machine& machine)
	{
		auto [fd, iov_addr, iovcnt] =
			machine.template sysargs<int, address_t, size_t>();
		if (iovcnt > 1024) {
			throw MachineException(ILLEGAL_OPERATION, "iovcnt too large in writev syscall");
		}

		if (fd == 1 || fd == 2) { // stdout or stderr
			size_t total = 0;
			for (size_t i = 0; i < iovcnt; i++) {
				// Each iovec is { void* iov_base, size_t iov_len }
				size_t iovec_offset = iov_addr + i * 2 * sizeof(address_t);
				auto base = machine.memory.template read<address_t>(iovec_offset);
				auto len = machine.memory.template read<address_t>(iovec_offset + sizeof(address_t));

				// Sanity check on length
				if (len > 0 && len < 1024 * 1024) {
					const char* view = machine.memory.template memarray<char>(base, len);
					machine.print(view, len);
					total += len;
				}
			}
			machine.set_result(total);
		} else {
			machine.set_result(-1);
		}
		sysprint(machine, "writev(fd=%d, iov=0x%llx, iovcnt=%llu) = %d\n",
			fd,
			static_cast<uint64_t>(iov_addr),
			static_cast<uint64_t>(iovcnt),
			machine.template return_value<int>());
	}

	// Read syscall (sandboxed - no host access)
	static void syscall_read(Machine& machine)
	{
		auto [fd, addr, len] =
			machine.template sysargs<int, address_t, size_t>();
		(void)addr;
		(void)len;

		// Sandboxed: stdin returns EOF, other fds return error
		if (fd == 0) {
			machine.set_result(0);  // EOF
		} else {
			machine.set_result(-LA_EBADF);
		}
		sysprint(machine, "read(fd=%d, buf=0x%llx, count=%llu) = %d\n",
			fd,
			static_cast<uint64_t>(addr),
			static_cast<uint64_t>(len),
			machine.template return_value<int>());
	}

	// Openat syscall (sandboxed - always fails)
	static void syscall_openat(Machine& machine)
	{
		(void)machine;
		machine.set_result(-LA_ENOENT);
	}

	// Close syscall
	static void syscall_close(Machine& machine)
	{
		auto [fd] = machine.template sysargs<int>();
		// Allow closing stdio descriptors (ignore silently)
		if (fd >= 0 && fd <= 2) {
			machine.set_result(0);
			return;
		}
		machine.set_result(-LA_EBADF);
	}

	// Fstat syscall (sandboxed)
	static void syscall_fstat(Machine& machine)
	{
		auto [fd, statbuf] =
			machine.template sysargs<int, address_t>();

		// Only support stdio descriptors
		if (fd >= 0 && fd <= 2 && statbuf != 0) {
			// Zero out the stat buffer (simplified)
			for (size_t i = 0; i < 128; i++) {
				machine.memory.template write<uint8_t>(statbuf + i, 0);
			}
			machine.set_result(0);
			return;
		}
		machine.set_result(-LA_EBADF);
	}
	// Fstatat syscall (sandboxed)
	static void syscall_fstatat(Machine& machine)
	{
		(void)machine;
		machine.set_result(-LA_ENOSYS);
	}

	// Ioctl syscall (sandboxed)
	static void syscall_ioctl(Machine& machine)
	{
		auto [fd] = machine.template sysargs<int>();
		// Return ENOTTY for stdio (not a terminal in sandbox)
		if (fd >= 0 && fd <= 2) {
			machine.set_result(-LA_ENOTTY);
			return;
		}
		machine.set_result(-LA_EBADF);
	}

	// Mprotect syscall
	static void syscall_mprotect(Machine& machine)
	{
		(void)machine;
		// Always succeed (memory protections not enforced in emulator)
		machine.set_result(0);
	}

	// Madvise syscall
	static void syscall_madvise(Machine& machine)
	{
		auto [addr, length, advice] =
			machine.template sysargs<address_t, size_t, int>();
		// Always succeed (advice is ignored)
		machine.set_result(0);
		sysprint(machine, "madvise(addr=0x%llx, len=%llu, advice=%d) = %d\n",
			static_cast<uint64_t>(addr), static_cast<uint64_t>(length), advice,
			machine.template return_value<int>());
	}

	// Clock_gettime syscall
	static void syscall_clock_gettime(Machine& machine)
	{
		auto [clockid, tp] =
			machine.template sysargs<int, address_t>();
		if (tp != 0) {
			struct timespec ts;
			clock_gettime(clockid, &ts);
			machine.memory.copy_to_guest(tp, &ts, sizeof(ts));
		}
		machine.set_result(0);
		sysprint(machine, "clock_gettime(clockid=%d, tp=0x%llx) = %d\n",
			clockid, static_cast<uint64_t>(tp),
			machine.template return_value<int>());
	}

	// gettimeofday syscall
	static void syscall_gettimeofday(Machine& machine)
	{
		auto [tv_addr] =
			machine.template sysargs<address_t>();
		if (tv_addr != 0) {
			struct timeval tv;
			gettimeofday(&tv, nullptr);
			machine.memory.copy_to_guest(tv_addr, &tv, sizeof(tv));
		}
		machine.set_result(0);
		sysprint(machine, "gettimeofday(tv=0x%llx) = %d\n",
			static_cast<uint64_t>(tv_addr),
			machine.template return_value<int>());
	}

	static void syscall_nanosleep(Machine& machine)
	{
		auto [req_addr, rem_addr] =
			machine.template sysargs<address_t, address_t>();
		if (req_addr != 0) {
			struct timespec req;
			machine.memory.copy_from_guest(&req, req_addr, sizeof(req));
			// In a real implementation, we would sleep here.
			// For the emulator, we just ignore the request.
			if (rem_addr != 0) {
				struct timespec rem = {0, 0};
				machine.memory.copy_to_guest(rem_addr, &rem, sizeof(rem));
			}
		}
		machine.set_result(0);
		sysprint(machine, "nanosleep(req=0x%llx, rem=0x%llx) = %d\n",
			static_cast<uint64_t>(req_addr),
			static_cast<uint64_t>(rem_addr),
			machine.template return_value<int>());
	}

	static void syscall_gettid(Machine& machine)
	{
		machine.set_result(machine.gettid());
		sysprint(machine, "gettid() = %d\n",
			machine.template return_value<int>());
	}

	static void syscall_sched_getaffinity(Machine& machine)
	{
		auto [pid, cpusetsize, mask_addr] =
			machine.template sysargs<int, size_t, address_t>();
		(void)pid;
		// For simplicity, assume single CPU (CPU 0)
		if (cpusetsize >= sizeof(uint64_t) && mask_addr != 0) {
			uint64_t mask = 1; // CPU 0
			machine.memory.copy_to_guest(mask_addr, &mask, sizeof(mask));
			machine.set_result(sizeof(uint64_t));
		} else {
			machine.set_result(-LA_EINVAL);
		}
		sysprint(machine, "sched_getaffinity(pid=%d, cpusetsize=%llu, mask=0x%llx) = %d\n",
			pid, static_cast<uint64_t>(cpusetsize),
			static_cast<uint64_t>(mask_addr),
			machine.template return_value<int>());
	}

	static void syscall_getpid(Machine& machine)
	{
		(void)machine;
		machine.set_result(0);  // Fake PID
	}

	static void syscall_getuid(Machine& machine)
	{
		(void)machine;
		machine.set_result(1000);  // Fake UID
	}

	// Linux signal action flags
	static constexpr int64_t LINUX_SA_ONSTACK = 0x08000000;

	static void syscall_sigaltstack(Machine& machine)
	{
		const auto ss = machine.sysarg(0);
		const auto old_ss = machine.sysarg(1);

		auto& stack = machine.signals().per_thread(machine.gettid()).stack;

		// Return old stack if requested
		if (old_ss != 0x0) {
			machine.memory.copy_to_guest(old_ss, &stack, sizeof(stack));
		}
		// Set new stack if provided
		if (ss != 0x0) {
			machine.memory.copy_from_guest(&stack, ss, sizeof(stack));
		}
		machine.set_result(0);
		sysprint(machine, "sigaltstack(ss=0x%llx, old_ss=0x%llx) = 0\n",
			static_cast<uint64_t>(ss), static_cast<uint64_t>(old_ss));
	}

	static void syscall_rt_sigaction(Machine& machine)
	{
		const int sig = machine.sysarg<int>(0);
		const auto action = machine.sysarg(1);
		const auto old_action = machine.sysarg(2);

		auto& sigact = machine.sigaction(sig);

		// Kernel sigaction structure (64-bit)
		struct kernel_sigaction {
			address_t sa_handler;
			address_t sa_flags;
			address_t sa_restorer;
			address_t sa_mask;
		};

		kernel_sigaction sa{};

		// Return old action if requested
		if (old_action != 0x0) {
			sa.sa_handler = sigact.handler & ~address_t(0x3);
			sa.sa_flags   = (sigact.altstack ? LINUX_SA_ONSTACK : 0x0);
			sa.sa_restorer = 0;
			sa.sa_mask    = sigact.mask;
			machine.memory.copy_to_guest(old_action, &sa, sizeof(sa));
		}
		// Set new action if provided
		if (action != 0x0) {
			machine.memory.copy_from_guest(&sa, action, sizeof(sa));
			sigact.handler  = sa.sa_handler & ~address_t(0x3);
			sigact.altstack = (sa.sa_flags & LINUX_SA_ONSTACK) != 0;
			sigact.mask     = sa.sa_mask;
		}
		machine.set_result(0);
		sysprint(machine, "rt_sigaction(sig=%d, action=0x%llx, old_action=0x%llx) = 0\n",
			sig, static_cast<uint64_t>(action), static_cast<uint64_t>(old_action));
	}

	static void syscall_rt_sigprocmask(Machine& machine)
	{
		// Stub: just return success
		machine.set_result(0);
		sysprint(machine, "rt_sigprocmask() = %d (stub)\n",
			machine.template return_value<int>());
	}

	static void syscall_tkill(Machine& machine)
	{
		const int tid = machine.sysarg<int>(0);
		const int sig = machine.sysarg<int>(1);

		sysprint(machine, "tkill(tid=%d, sig=%d)\n", tid, sig);

		// If the signal is zero or unset, ignore it
		if (sig == 0 || machine.sigaction(sig).is_unset()) {
			machine.set_result(0);
			return;
		}

		// Jump to signal handler and change to altstack, if set
		machine.signals().enter(machine, sig);
		machine.set_result(0);
	}

	static void syscall_tgkill(Machine& machine)
	{
		const int tgid = machine.sysarg<int>(0);
		const int tid = machine.sysarg<int>(1);
		const int sig = machine.sysarg<int>(2);

		// Signal 6 is SIGABRT - special handling
		if (sig == 6) {
			sysprint(machine, "tgkill(tgid=%d, tid=%d, sig=%d) - aborting\n", tgid, tid, sig);
			// Check if there's a handler set
			if (machine.sigaction(sig).is_unset()) {
				// No handler - program called abort(), terminate
				throw MachineException(GUEST_ABORT, "Program aborted via abort()");
			}
			// There's a handler - enter it
		}

		sysprint(machine, "tgkill(tgid=%d, tid=%d, sig=%d)\n", tgid, tid, sig);

		// If the signal is zero or unset, ignore it
		if (sig == 0 || machine.sigaction(sig).is_unset()) {
			machine.set_result(0);
			return;
		}

		// Jump to signal handler and change to altstack, if set
		machine.signals().enter(machine, sig);
		machine.set_result(0);
	}

	static void syscall_brk(Machine& machine)
	{
		auto new_end = machine.cpu.reg(REG_A0);
		static const address_t BRK_MAX = 0x100000; // XXX: Fake
		/// XXX: There is something wrong about brk() emulation.
		machine.set_result(0);
		return;

		// Clamp new_end within valid range
		const auto brk_start = machine.memory.brk_address();
		if (new_end > brk_start + BRK_MAX) {
			new_end = brk_start + BRK_MAX;
		} else if (new_end < brk_start) {
			new_end = brk_start;
		}

		// Set new brk
		machine.set_result(new_end);
		sysprint(machine, "brk(0x%llx) = 0x%llx\n",
			static_cast<uint64_t>(new_end),
			static_cast<uint64_t>(machine.template return_value<address_t>()));
	}

	static void syscall_fcntl(Machine& machine)
	{
		// int fd = machine.cpu.reg(REG_A0);
		// int cmd = machine.cpu.reg(REG_A1);
		// Stub implementation: return 0 (success)
		(void)machine;
		machine.set_result(0);
	}

	static void syscall_set_tid_address(Machine& machine)
	{
		// Return thread ID
		machine.set_result(machine.gettid());
	}

	static void syscall_set_robust_list(Machine& machine)
	{
		// This is used for robust futexes; we can ignore for single-threaded emulation
		//machine.cpu.reg(REG_A0);  // head pointer
		//machine.cpu.reg(REG_A1);  // len
		// Return success
		machine.set_result(0);
	}

	static void syscall_readlinkat(Machine& machine)
	{
		// int dirfd = machine.cpu.reg(REG_A0);  // AT_FDCWD = -100
		const address_t pathname_addr = machine.cpu.reg(REG_A1);
		const address_t buf_addr = machine.cpu.reg(REG_A2);
		const size_t bufsiz = machine.cpu.reg(REG_A3);

		// Read the pathname
		std::string pathname = machine.memory.memstring(pathname_addr, 256);

		// For /proc/self/exe, return a fake path
		std::string result;
		if (pathname == "/proc/self/exe") {
			result = "/tmp/program";  // Fake executable path
		} else {
			// Return ENOENT for other paths
			machine.set_result(-LA_ENOENT);
			return;
		}

		// Copy result to buffer
		const size_t len = std::min(result.size(), bufsiz);
		machine.memory.copy_to_guest(buf_addr, result.data(), len);
		machine.set_result(len);
	}

	static void syscall_getrandom(Machine& machine)
	{
		auto buf_addr = machine.cpu.reg(REG_A0);
		size_t buflen = machine.cpu.reg(REG_A1);
		// int flags = machine.cpu.reg(REG_A2);

		// Fill buffer with pseudo-random data
		for (size_t i = 0; i < buflen; i++) {
			machine.memory.template write<uint8_t>(buf_addr + i, static_cast<uint8_t>(i * 17 + 31));
		}

		machine.set_result(buflen);
	}

	static void syscall_prlimit64(Machine& machine)
	{
		// pid_t pid = machine.cpu.reg(REG_A0);
		int resource = machine.cpu.reg(REG_A1);
		// const struct rlimit64 *new_limit = machine.cpu.reg(REG_A2);
		auto old_limit = machine.cpu.reg(REG_A3);

		// If old_limit is provided, fill in with reasonable defaults
		if (old_limit != 0) {
			// struct rlimit64 { uint64_t rlim_cur; uint64_t rlim_max; }
			uint64_t soft_limit = 0;
			uint64_t hard_limit = 0;

			// Provide reasonable fake values
			switch (resource) {
				case 3:  // RLIMIT_STACK
					soft_limit = 8 * 1024 * 1024;  // 8 MB
					hard_limit = UINT64_MAX;
					break;
				case 7:  // RLIMIT_NOFILE
					soft_limit = 1024;
					hard_limit = 4096;
					break;
				default:
					soft_limit = UINT64_MAX;
					hard_limit = UINT64_MAX;
					break;
			}

			machine.memory.template write<uint64_t>(old_limit, soft_limit);
			machine.memory.template write<uint64_t>(old_limit + 8, hard_limit);
		}

		machine.set_result(0);  // Success
		sysprint(machine, "prlimit64(pid=%d, resource=%d, new_limit=0x%llx, old_limit=0x%llx) = %d\n",
			machine.cpu.reg(REG_A0), resource,
			static_cast<uint64_t>(machine.cpu.reg(REG_A2)),
			static_cast<uint64_t>(old_limit),
			machine.template return_value<int>());
	}

	// MMAP syscall
	static void syscall_mmap(Machine& machine)
	{
		auto [addr, length, prot, flags, fd, offset] =
			machine.template sysargs<address_t, size_t, int, int, int, off_t>();

		// Simple implementation: allocate memory from our heap
		if (addr == 0) {
			// Anonymous mapping - allocate new memory
			auto new_addr = machine.memory.mmap_allocate(length);
			machine.set_result(new_addr);
		} else if ((flags & 0x10) == 0x0) {
			// no fixed flag - force into new allocation
			auto new_addr = machine.memory.mmap_allocate(length);
			machine.set_result(new_addr);
		} else if (addr < machine.memory.mmap_address()) {
			// It's within the mmap region - allow
			machine.set_result(addr);
		} else {
			// Fixed address mapping not supported for now
			machine.set_result(-1);
		}

		// Anonymous mapping: zero out the memory
		if (false && machine.template return_value<address_t>() != static_cast<address_t>(-1)) {
			if (flags & 0x20) { // MAP_ANONYMOUS
				machine.memory.memset(machine.template return_value<address_t>(), 0, length);
			}
		}

		sysprint(machine, "mmap(addr=0x%llx, len=%llu, prot=0x%x, flags=0x%x, fd=%d, offset=%llu) = 0x%llx\n",
			static_cast<uint64_t>(addr), static_cast<uint64_t>(length), prot, flags, fd,
			static_cast<uint64_t>(offset),
			static_cast<uint64_t>(machine.cpu.reg(REG_A0)));
	}
	static void syscall_munmap(Machine& machine)
	{
		const auto addr  = machine.cpu.reg(REG_A0);
		const auto length = machine.cpu.reg(REG_A1);

		machine.memory.mmap_deallocate(addr, length);
		machine.set_result(0);
		sysprint(machine, "munmap(addr=0x%llx, len=%llu) = %d\n",
			static_cast<uint64_t>(addr), static_cast<uint64_t>(length),
			machine.template return_value<int>());
	}

	// Futex syscall (basic support for threading)
	static void syscall_futex(Machine& machine)
	{
		// int* uaddr = machine.cpu.reg(REG_A0);
		int futex_op = machine.cpu.reg(REG_A1);
		// int val = machine.cpu.reg(REG_A2);

		// Basic futex operations
		constexpr int FUTEX_WAIT = 0;
		constexpr int FUTEX_WAKE = 1;
		constexpr int FUTEX_PRIVATE_FLAG = 128;

		int op = futex_op & ~FUTEX_PRIVATE_FLAG;

		switch (op) {
			case FUTEX_WAIT:
				// In single-threaded mode, waiting would block forever
				// Return EAGAIN to indicate the value changed
				machine.set_result(-LA_EAGAIN);
				return;
			case FUTEX_WAKE:
				// No threads to wake
				machine.set_result(0);
				return;
			default:
				machine.set_result(static_cast<address_t>(-LA_ENOSYS));
				return;
		}
	}

	// PRCTL syscall - process control
	static void syscall_prctl(Machine& machine)
	{
		int option = machine.cpu.reg(REG_A0);
		sysprint(machine, "prctl(option=%d, ...) = 0 (stub)\n", option);
		// Return success for most operations
		machine.set_result(0);
	}

	// PPOLL syscall (mark stdio as ready)
	static void syscall_ppoll(Machine& machine)
	{
		auto [fds_addr, nfds] =
			machine.template sysargs<address_t, size_t>();
		// const auto timeout_addr = machine.cpu.reg(REG_A2);
		if (nfds > 1024) {
			throw MachineException(ILLEGAL_OPERATION, "nfds too large in ppoll syscall");
		}
		struct vpollfd {
			int fd;
			short events;
			short revents;
		};
		vpollfd* fds = machine.memory.template writable_memarray<vpollfd>(fds_addr, nfds);
		for (size_t i = 0; i < nfds; i++) {
			const int fd = fds[i].fd;
			// Only handle stdio fds
			if (fd >= 0 && fd <= 2) {
				// Set revents to match events (ready)
				auto events = fds[i].events;
				fds[i].revents = events;
			} else {
				// Clear revents for other fds
				fds[i].revents = 0;
			}
		}
		// Mark stdio as ready (stub)
		machine.set_result(0);
	}

	void Machine::setup_linux_syscalls()
	{
		// Process lifecycle
		install_syscall_handler(LA_SYS_exit, syscall_exit);
		install_syscall_handler(LA_SYS_exit_group, syscall_exit);
		// I/O (sandboxed)
		install_syscall_handler(LA_SYS_write, syscall_write);
		install_syscall_handler(LA_SYS_writev, syscall_writev);
		install_syscall_handler(LA_SYS_read, syscall_read);
		install_syscall_handler(LA_SYS_openat, syscall_openat);
		install_syscall_handler(LA_SYS_close, syscall_close);
		install_syscall_handler(LA_SYS_fstat, syscall_fstat);
		install_syscall_handler(LA_SYS_ioctl, syscall_ioctl);
		install_syscall_handler(LA_SYS_fcntl, syscall_fcntl);
		install_syscall_handler(LA_SYS_readlinkat, syscall_readlinkat);
		install_syscall_handler(LA_SYS_fstatat, syscall_fstatat);
		install_syscall_handler(LA_SYS_ppoll, syscall_ppoll);

		// Memory management
		install_syscall_handler(LA_SYS_brk, syscall_brk);
		install_syscall_handler(LA_SYS_mmap, syscall_mmap);
		install_syscall_handler(LA_SYS_mprotect, syscall_mprotect);
		install_syscall_handler(LA_SYS_madvise, syscall_madvise);
		install_syscall_handler(LA_SYS_munmap, syscall_munmap);

		// Threading/synchronization
		install_syscall_handler(LA_SYS_set_tid_address, syscall_set_tid_address);
		install_syscall_handler(LA_SYS_set_robust_list, syscall_set_robust_list);
		install_syscall_handler(LA_SYS_futex, syscall_futex);
		install_syscall_handler(LA_SYS_gettid, syscall_gettid);
		install_syscall_handler(LA_SYS_sched_getaffinity, syscall_sched_getaffinity);

		// Process info
		install_syscall_handler(LA_SYS_getpid, syscall_getpid);
		install_syscall_handler(LA_SYS_getuid, syscall_getuid);
		install_syscall_handler(LA_SYS_geteuid, syscall_getuid);
		install_syscall_handler(LA_SYS_getgid, syscall_getuid);
		install_syscall_handler(LA_SYS_getegid, syscall_getuid);

		// Resource limits
		install_syscall_handler(LA_SYS_prlimit64, syscall_prlimit64);

		// Time
		install_syscall_handler(LA_SYS_clock_gettime, syscall_clock_gettime);
		install_syscall_handler(LA_SYS_gettimeofday, syscall_gettimeofday);
		install_syscall_handler(LA_SYS_nanosleep, syscall_nanosleep);

		// Signals
		install_syscall_handler(LA_SYS_tkill, syscall_tkill);
		install_syscall_handler(LA_SYS_tgkill, syscall_tgkill);
		install_syscall_handler(LA_SYS_sigaltstack, syscall_sigaltstack);
		install_syscall_handler(LA_SYS_rt_sigaction, syscall_rt_sigaction);
		install_syscall_handler(LA_SYS_rt_sigprocmask, syscall_rt_sigprocmask);

		// Other
		install_syscall_handler(LA_SYS_getrandom, syscall_getrandom);

		// Process control
		install_syscall_handler(LA_SYS_prctl, syscall_prctl);
	}

	void Machine::setup_minimal_syscalls()
	{
		// Setup basic syscalls for Newlib support
		throw MachineException(FEATURE_DISABLED, "Minimal syscall setup not implemented yet");
	}

} // loongarch
