#include "../machine.hpp"
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
	enum LA_Syscalls {
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
		LA_SYS_rt_sigaction = 134,
		LA_SYS_rt_sigprocmask = 135,
		LA_SYS_madvise = 233,
		LA_SYS_tgkill = 131,
		LA_SYS_prctl = 167,
		LA_SYS_fstatat = 291,
	};

	template <int W, typename... Args>
	static inline void sysprint(Machine<W>& machine, const char* fmt, Args... args)
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
	template <int W>
	static void syscall_exit(Machine<W>& machine)
	{
		machine.stop();
		sysprint(machine, "exit(status=%d)\n",
			machine.template return_value<int>());
	}

	// Write syscall
	template <int W>
	static void syscall_write(Machine<W>& machine)
	{
		int fd = machine.cpu.reg(REG_A0);
		auto addr = machine.cpu.reg(REG_A1);
		size_t len = machine.cpu.reg(REG_A2);

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
	template <int W>
	static void syscall_writev(Machine<W>& machine)
	{
		int fd = machine.cpu.reg(REG_A0);
		auto iov_addr = machine.cpu.reg(REG_A1);
		size_t iovcnt = machine.cpu.reg(REG_A2);
		if (iovcnt > 1024) {
			throw MachineException(ILLEGAL_OPERATION, "iovcnt too large in writev syscall");
		}

		if (fd == 1 || fd == 2) { // stdout or stderr
			size_t total = 0;
			for (size_t i = 0; i < iovcnt; i++) {
				// Each iovec is { void* iov_base, size_t iov_len }
				size_t iovec_offset = iov_addr + i * 2 * sizeof(address_type<W>);
				auto base = machine.memory.template read<address_type<W>>(iovec_offset);
				auto len = machine.memory.template read<address_type<W>>(iovec_offset + sizeof(address_type<W>));

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
	template <int W>
	static void syscall_read(Machine<W>& machine)
	{
		int fd = machine.cpu.reg(REG_A0);
		(void)machine.cpu.reg(REG_A1);  // addr
		(void)machine.cpu.reg(REG_A2);  // len

		// Sandboxed: stdin returns EOF, other fds return error
		if (fd == 0) {
			machine.set_result(0);  // EOF
		} else {
			machine.set_result(-LA_EBADF);
		}
		sysprint(machine, "read(fd=%d, buf=0x%llx, count=%llu) = %d\n",
			fd,
			static_cast<uint64_t>(machine.cpu.reg(REG_A1)),
			static_cast<uint64_t>(machine.cpu.reg(REG_A2)),
			machine.template return_value<int>());
	}

	// Openat syscall (sandboxed - always fails)
	template <int W>
	static void syscall_openat(Machine<W>& machine)
	{
		(void)machine;
		machine.set_result(-LA_ENOENT);
	}

	// Close syscall
	template <int W>
	static void syscall_close(Machine<W>& machine)
	{
		int fd = machine.cpu.reg(REG_A0);
		// Allow closing stdio descriptors (ignore silently)
		if (fd >= 0 && fd <= 2) {
			machine.set_result(0);
			return;
		}
		machine.set_result(-LA_EBADF);
	}

	// Fstat syscall (sandboxed)
	template <int W>
	static void syscall_fstat(Machine<W>& machine)
	{
		int fd = machine.cpu.reg(REG_A0);
		auto statbuf = machine.cpu.reg(REG_A1);

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
	template <int W>
	static void syscall_fstatat(Machine<W>& machine)
	{
		(void)machine;
		machine.set_result(-LA_ENOSYS);
	}

	// Ioctl syscall (sandboxed)
	template <int W>
	static void syscall_ioctl(Machine<W>& machine)
	{
		int fd = machine.cpu.reg(REG_A0);
		// Return ENOTTY for stdio (not a terminal in sandbox)
		if (fd >= 0 && fd <= 2) {
			machine.set_result(-LA_ENOTTY);
			return;
		}
		machine.set_result(-LA_EBADF);
	}

	// Mprotect syscall
	template <int W>
	static void syscall_mprotect(Machine<W>& machine)
	{
		(void)machine;
		// Always succeed (memory protections not enforced in emulator)
		machine.set_result(0);
	}

	// Madvise syscall
	template <int W>
	static void syscall_madvise(Machine<W>& machine)
	{
		const auto addr = machine.cpu.reg(REG_A0);
		const size_t length = machine.cpu.reg(REG_A1);
		const int advice = machine.cpu.reg(REG_A2);
		// Always succeed (advice is ignored)
		machine.set_result(0);
		sysprint(machine, "madvise(addr=0x%llx, len=%llu, advice=%d) = %d\n",
			static_cast<uint64_t>(addr), static_cast<uint64_t>(length), advice,
			machine.template return_value<int>());
	}

	// Clock_gettime syscall
	template <int W>
	static void syscall_clock_gettime(Machine<W>& machine)
	{
		const int clockid = machine.cpu.reg(REG_A0);
		const address_type<W> tp = machine.cpu.reg(REG_A1);
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
	template <int W>
	static void syscall_gettimeofday(Machine<W>& machine)
	{
		const address_type<W> tv_addr = machine.cpu.reg(REG_A0);
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

	// Gettid syscall
	template <int W>
	static void syscall_gettid(Machine<W>& machine)
	{
		machine.set_result(machine.gettid());
		sysprint(machine, "gettid() = %d\n",
			machine.template return_value<int>());
	}

	// Getpid syscall
	template <int W>
	static void syscall_getpid(Machine<W>& machine)
	{
		(void)machine;
		machine.set_result(1);  // Fake PID
	}

	// Getuid/Geteuid/Getgid/Getegid syscalls
	template <int W>
	static void syscall_getuid(Machine<W>& machine)
	{
		(void)machine;
		machine.set_result(1000);  // Fake UID
	}

	// Rt_sigaction syscall
	template <int W>
	static void syscall_rt_sigaction(Machine<W>& machine)
	{
		(void)machine;
		// Ignore signal handlers in sandbox
		machine.set_result(0);
	}

	// Rt_sigprocmask syscall
	template <int W>
	static void syscall_rt_sigprocmask(Machine<W>& machine)
	{
		(void)machine;
		// Ignore signal masks in sandbox
		machine.set_result(0);
	}

	// BRK syscall
	template <int W>
	static void syscall_brk(Machine<W>& machine)
	{
		auto new_end = machine.cpu.reg(REG_A0);
		static const address_type<W> BRK_MAX = 0x100000; // XXX: Fake
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
			static_cast<uint64_t>(machine.template return_value<address_type<W>>()));
	}

	// FCNTL syscall
	template <int W>
	static void syscall_fcntl(Machine<W>& machine)
	{
		// int fd = machine.cpu.reg(REG_A0);
		// int cmd = machine.cpu.reg(REG_A1);
		// Stub implementation: return 0 (success)
		(void)machine;
		machine.set_result(0);
	}

	// SET_TID_ADDRESS syscall
	template <int W>
	static void syscall_set_tid_address(Machine<W>& machine)
	{
		// Store the address for thread exit notification
		const address_type<W> tidptr = machine.cpu.reg(REG_A0);
		machine.set_tid_address(tidptr);
		// Return thread ID
		machine.set_result(machine.gettid());
	}

	// SET_ROBUST_LIST syscall
	template <int W>
	static void syscall_set_robust_list(Machine<W>& machine)
	{
		// This is used for robust futexes; we can ignore for single-threaded emulation
		//machine.cpu.reg(REG_A0);  // head pointer
		//machine.cpu.reg(REG_A1);  // len
		// Return success
		machine.set_result(0);
	}

	// READLINKAT syscall
	template <int W>
	static void syscall_readlinkat(Machine<W>& machine)
	{
		// int dirfd = machine.cpu.reg(REG_A0);  // AT_FDCWD = -100
		const address_type<W> pathname_addr = machine.cpu.reg(REG_A1);
		const address_type<W> buf_addr = machine.cpu.reg(REG_A2);
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

	// GETRANDOM syscall
	template <int W>
	static void syscall_getrandom(Machine<W>& machine)
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

	// PRLIMIT64 syscall (getrlimit/setrlimit)
	template <int W>
	static void syscall_prlimit64(Machine<W>& machine)
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
	template <int W>
	static void syscall_mmap(Machine<W>& machine)
	{
		const auto addr  = machine.cpu.reg(REG_A0);
		const auto length = machine.cpu.reg(REG_A1);
		const int  prot  = machine.cpu.reg(REG_A2);
		const int  flags = machine.cpu.reg(REG_A3);
		const int  fd    = machine.cpu.reg(REG_A4);
		const off_t offset = machine.cpu.reg(REG_A5);

		// Simple implementation: allocate memory from our heap
		if (addr == 0) {
			// Anonymous mapping - allocate new memory
			auto new_addr = machine.memory.mmap_allocate(length);
			machine.set_result(new_addr);
		} else {
			// Fixed address mapping not supported for now
			machine.set_result(-1);
		}
		sysprint(machine, "mmap(addr=0x%llx, len=%llu, prot=0x%x, flags=0x%x, fd=%d, offset=%llu) = 0x%llx\n",
			static_cast<uint64_t>(addr), static_cast<uint64_t>(length), prot, flags, fd,
			static_cast<uint64_t>(offset),
			static_cast<uint64_t>(machine.cpu.reg(REG_A0)));
	}

	// Futex syscall (basic support for threading)
	template <int W>
	static void syscall_futex(Machine<W>& machine)
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
				machine.set_result(static_cast<address_type<W>>(-LA_ENOSYS));
				return;
		}
	}

	// TGKILL syscall - used by abort() to send signal
	template <int W>
	static void syscall_tgkill(Machine<W>& machine)
	{
		int tgid = machine.cpu.reg(REG_A0);
		int tid = machine.cpu.reg(REG_A1);
		int sig = machine.cpu.reg(REG_A2);

		sysprint(machine, "tgkill(tgid=%d, tid=%d, sig=%d) - aborting\n", tgid, tid, sig);

		// Signal 6 is SIGABRT
		if (sig == 6) {
			// Program called abort()
			throw MachineException(GUEST_ABORT, "Program aborted via abort()");
		} else {
			machine.set_result(-LA_ENOSYS);
		}
	}

	// PRCTL syscall - process control
	template <int W>
	static void syscall_prctl(Machine<W>& machine)
	{
		int option = machine.cpu.reg(REG_A0);
		sysprint(machine, "prctl(option=%d, ...) = 0 (stub)\n", option);
		// Return success for most operations
		machine.set_result(0);
	}

	// PPOLL syscall (mark stdio as ready)
	template <int W>
	static void syscall_ppoll(Machine<W>& machine)
	{
		const auto fds_addr = machine.cpu.reg(REG_A0);
		const size_t nfds = machine.cpu.reg(REG_A1);
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

	template <int W>
	void Machine<W>::setup_linux_syscalls()
	{
		// Process lifecycle
		install_syscall_handler(LA_SYS_exit, syscall_exit<W>);
		install_syscall_handler(LA_SYS_exit_group, syscall_exit<W>);
		// I/O (sandboxed)
		install_syscall_handler(LA_SYS_write, syscall_write<W>);
		install_syscall_handler(LA_SYS_writev, syscall_writev<W>);
		install_syscall_handler(LA_SYS_read, syscall_read<W>);
		install_syscall_handler(LA_SYS_openat, syscall_openat<W>);
		install_syscall_handler(LA_SYS_close, syscall_close<W>);
		install_syscall_handler(LA_SYS_fstat, syscall_fstat<W>);
		install_syscall_handler(LA_SYS_ioctl, syscall_ioctl<W>);
		install_syscall_handler(LA_SYS_fcntl, syscall_fcntl<W>);
		install_syscall_handler(LA_SYS_readlinkat, syscall_readlinkat<W>);
		install_syscall_handler(LA_SYS_fstatat, syscall_fstatat<W>);
		install_syscall_handler(LA_SYS_ppoll, syscall_ppoll<W>);

		// Memory management
		install_syscall_handler(LA_SYS_brk, syscall_brk<W>);
		install_syscall_handler(LA_SYS_mmap, syscall_mmap<W>);
		install_syscall_handler(LA_SYS_mprotect, syscall_mprotect<W>);
		install_syscall_handler(LA_SYS_madvise, syscall_madvise<W>);

		// Threading/synchronization
		install_syscall_handler(LA_SYS_set_tid_address, syscall_set_tid_address<W>);
		install_syscall_handler(LA_SYS_set_robust_list, syscall_set_robust_list<W>);
		install_syscall_handler(LA_SYS_futex, syscall_futex<W>);
		install_syscall_handler(LA_SYS_gettid, syscall_gettid<W>);

		// Process info
		install_syscall_handler(LA_SYS_getpid, syscall_getpid<W>);
		install_syscall_handler(LA_SYS_getuid, syscall_getuid<W>);
		install_syscall_handler(LA_SYS_geteuid, syscall_getuid<W>);
		install_syscall_handler(LA_SYS_getgid, syscall_getuid<W>);
		install_syscall_handler(LA_SYS_getegid, syscall_getuid<W>);

		// Resource limits
		install_syscall_handler(LA_SYS_prlimit64, syscall_prlimit64<W>);

		// Time
		install_syscall_handler(LA_SYS_clock_gettime, syscall_clock_gettime<W>);
		install_syscall_handler(LA_SYS_gettimeofday, syscall_gettimeofday<W>);

		// Signals (ignored in sandbox)
		install_syscall_handler(LA_SYS_rt_sigaction, syscall_rt_sigaction<W>);
		install_syscall_handler(LA_SYS_rt_sigprocmask, syscall_rt_sigprocmask<W>);

		// Other
		install_syscall_handler(LA_SYS_getrandom, syscall_getrandom<W>);

		// Signals (abort handling)
		install_syscall_handler(LA_SYS_tgkill, syscall_tgkill<W>);

		// Process control
		install_syscall_handler(LA_SYS_prctl, syscall_prctl<W>);
	}

	template <int W>
	void Machine<W>::setup_minimal_syscalls()
	{
		// Setup basic syscalls for Newlib support
		throw MachineException(FEATURE_DISABLED, "Minimal syscall setup not implemented yet");
	}

#ifdef LA_32
	template void Machine<LA32>::setup_linux_syscalls();
	template void Machine<LA32>::setup_minimal_syscalls();
#endif
#ifdef LA_64
	template void Machine<LA64>::setup_linux_syscalls();
	template void Machine<LA64>::setup_minimal_syscalls();
#endif

} // loongarch
