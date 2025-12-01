#include "machine.hpp"
#include "threaded_bytecodes.hpp"
#include "native/heap.hpp"
#include <cstdio>
#include <inttypes.h>

//#define VERBOSE_NATSYS
#ifdef VERBOSE_NATSYS
#define HPRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define HPRINT(fmt, ...) /* */
#endif

namespace loongarch {

static constexpr int SYS_native_memmove = 511;
static constexpr int SYS_native_memcmp  = 510;
static constexpr int SYS_native_memset  = 509;
static constexpr int SYS_native_memcpy  = 508;
static constexpr int SYS_native_memchr  = 507;
static constexpr int SYS_native_strncmp = 503;
static constexpr int SYS_native_strcmp  = 502;
static constexpr int SYS_native_strnlen = 501;
static constexpr int SYS_native_strlen  = 500;
static constexpr bool VERBOSE_PATCHING = false;

static inline void patch(Machine& machine, const std::vector<std::string> symbols, const int syscall_number)
{
	for (const auto& sym : symbols) {
		const auto addr = machine.address_of(sym);
		if (addr != 0) {
			if constexpr (VERBOSE_PATCHING) {
				printf("Patching accelerated syscall for %s at 0x%" PRIx64 "\n",
					sym.c_str(), static_cast<uint64_t>(addr));
			}
			// Patch the function prologue to invoke the syscall
			// using syscall.imm bytecode
			DecoderData entry;
			entry.bytecode = LA64_BC_SYSCALLIMM;
			entry.handler_idx = 0; // Invalid
			entry.block_bytes = 0; // Diverges here
			entry.instr = syscall_number;
			// Install into decoder cache
			auto& exec_seg = *machine.memory.exec_segment_for(addr);
			exec_seg.set(addr, entry);
		}
	}
}

void Machine::setup_accelerated_syscalls()
{
	// Register accelerated syscalls here
	// The goal is to:
	// 1. Detect public symbols like memcpy, __memcpy etc.
	// 2. Write a system call rigmarole into the current
	//    decoder cache overwriting ~2-4 instructions at the symbol address.
	// 3. When the syscall is invoked, it jumps into the emulator's
	//    accelerated syscall handler which performs the operation natively.

	install_syscall_handler(SYS_native_memcpy,
	[] (Machine& machine) {
		const address_t dest = machine.cpu.reg(REG_A0);
		const address_t src  = machine.cpu.reg(REG_A1);
		const size_t n       = machine.cpu.reg(REG_A2);
		if (LA_UNLIKELY(n == 0)) {
			machine.set_result(dest);
			return;
		}
		// Perform native memcpy
		uint8_t* dest_ptr = machine.memory.template writable_memarray<uint8_t>(dest, n);
		const uint8_t* src_ptr = machine.memory.template memarray<uint8_t>(src, n);
		std::memcpy(dest_ptr, src_ptr, n);

		machine.set_result(dest);
	});

	install_syscall_handler(SYS_native_memset,
	[] (Machine& machine) {
		const address_t dest = machine.cpu.reg(REG_A0);
		const int value      = machine.cpu.reg(REG_A1);
		const size_t n       = machine.cpu.reg(REG_A2);
		// Perform native memset
		uint8_t* dest_ptr = machine.memory.template writable_memarray<uint8_t>(dest, n);
		std::memset(dest_ptr, value, n);

		machine.set_result(dest);
	});

	install_syscall_handler(SYS_native_memcmp,
	[] (Machine& machine) {
		const address_t ptr1 = machine.cpu.reg(REG_A0);
		const address_t ptr2 = machine.cpu.reg(REG_A1);
		const size_t n       = machine.cpu.reg(REG_A2);
		// Perform native memcmp
		const uint8_t* p1 = machine.memory.template memarray<uint8_t>(ptr1, n);
		const uint8_t* p2 = machine.memory.template memarray<uint8_t>(ptr2, n);
		int result = std::memcmp(p1, p2, n);

		machine.set_result<int>(result);
	});

	install_syscall_handler(SYS_native_memmove,
	[] (Machine& machine) {
		const address_t dest = machine.cpu.reg(REG_A0);
		const address_t src  = machine.cpu.reg(REG_A1);
		const size_t n       = machine.cpu.reg(REG_A2);
		// Perform native memmove
		uint8_t* dest_ptr = machine.memory.template writable_memarray<uint8_t>(dest, n);
		const uint8_t* src_ptr = machine.memory.template memarray<uint8_t>(src, n);
		std::memmove(dest_ptr, src_ptr, n);

		machine.set_result(dest);
	});

	install_syscall_handler(SYS_native_memchr,
	[] (Machine& machine) {
		const address_t ptr = machine.cpu.reg(REG_A0);
		const int value     = machine.cpu.reg(REG_A1);
		const size_t n      = machine.cpu.reg(REG_A2);
		// Perform native memchr
		const uint8_t* p = machine.memory.template memarray<uint8_t>(ptr, n);
		const void* result = std::memchr(p, value, n);

		if (result) {
			const address_t offset = static_cast<const uint8_t*>(result) - p;
			machine.set_result(ptr + offset);
		} else {
			machine.set_result<address_t>(0);
		}
	});

	install_syscall_handler(SYS_native_strlen,
	[] (Machine& machine) {
		const address_t str_addr = machine.cpu.reg(REG_A0);
		machine.set_result(machine.memory.strlen(str_addr));
	});

	install_syscall_handler(SYS_native_strnlen,
	[] (Machine& machine) {
		const address_t str_addr = machine.cpu.reg(REG_A0);
		const size_t maxlen      = machine.cpu.reg(REG_A1);
		machine.set_result(machine.memory.strlen(str_addr, maxlen));
	});

	install_syscall_handler(SYS_native_strcmp,
	[] (Machine& machine) {
		const address_t str1_addr = machine.cpu.reg(REG_A0);
		const address_t str2_addr = machine.cpu.reg(REG_A1);
		const size_t len1 = machine.memory.strlen(str1_addr);
		const size_t len2 = machine.memory.strlen(str2_addr);
		const size_t cmp_len = std::min(len1, len2);
		const char* s1 = machine.memory.template memarray<char>(str1_addr, cmp_len + 1);
		const char* s2 = machine.memory.template memarray<char>(str2_addr, cmp_len + 1);
		int result = std::memcmp(s1, s2, cmp_len);
		if (result == 0) {
			if (len1 < len2) {
				result = -1;
			} else if (len1 > len2) {
				result = 1;
			}
		}

		machine.set_result<int>(result);
	});

	install_syscall_handler(SYS_native_strncmp,
	[] (Machine& machine) {
		const address_t str1_addr = machine.cpu.reg(REG_A0);
		const address_t str2_addr = machine.cpu.reg(REG_A1);
		const size_t n            = machine.cpu.reg(REG_A2);
		const char* s1 = machine.memory.template memarray<char>(str1_addr, n);
		const char* s2 = machine.memory.template memarray<char>(str2_addr, n);
		const int result = std::memcmp(s1, s2, n);

		machine.set_result<int>(result);
	});

	// Iterate the symbol table and patch known functions
	patch(*this, {"__memcpy_lsx", "__memcpy_lasx", "__memcpy_aligned", "__memcpy_unaligned"}, SYS_native_memcpy);
	patch(*this, {"__memset_lsx", "__memset_lasx", "__memset_aligned", "__memset_unaligned"}, SYS_native_memset);
	patch(*this, {"__memcmp_lsx", "__memcmp_lasx", "__memcmp_aligned"}, SYS_native_memcmp);
	patch(*this, {"__memmove_lsx", "__memmove_lasx", "__memmove_aligned", "__memmove_unaligned"}, SYS_native_memmove);
	patch(*this, {"__memchr_lsx", "__memchr_lasx", "__memchr_aligned"}, SYS_native_memchr);
	patch(*this, {"__strlen_lsx", "__strlen_lasx", "__strlen_aligned"}, SYS_native_strlen);
	patch(*this, {"__strnlen_lsx", "__strnlen_lasx", "__strnlen_aligned"}, SYS_native_strnlen);
	patch(*this, {"__strcmp_lsx", "__strcmp_lasx", "__strcmp_aligned"}, SYS_native_strcmp);
	patch(*this, {"__strncmp_lsx", "__strncmp_lasx", "__strncmp_aligned"}, SYS_native_strncmp);
}

void Machine::setup_accelerated_heap(address_t arena_base, size_t arena_size)
{
	if (!this->has_arena()) {
		this->m_arena = std::make_unique<Arena>(arena_base, arena_base + arena_size);
	}

	// Setup accelerated malloc/free/realloc/calloc syscalls
	const size_t syscall_base = 495; // Arbitrary high number to avoid conflicts

	// Malloc n+0
	install_syscall_handler(syscall_base+0,
	[] (Machine& machine)
	{
		const size_t len = machine.sysarg(0);
		auto data = machine.arena().malloc(len);
		HPRINT("SYSCALL malloc(%zu) = 0x%lX\n", len, (long)data);
		machine.set_result(data);
	});
	// Calloc n+1
	install_syscall_handler(syscall_base+1,
	[] (Machine& machine)
	{
		const auto [count, size] =
			machine.template sysargs<address_t, address_t> ();
		const size_t len = count * size;
		auto data = machine.arena().malloc(len);
		HPRINT("SYSCALL calloc(%zu, %zu) = 0x%lX\n",
			(size_t)count, (size_t)size, (long)data);
		if (data != 0) {
			machine.memory.memset(data, 0, len);
		}
		machine.set_result(data);
	});
	// Realloc n+2
	install_syscall_handler(syscall_base+2,
	[] (Machine& machine)
	{
		const auto src = machine.sysarg(0);
		const auto newlen = machine.sysarg(1);

		const auto [data, srclen] = machine.arena().realloc(src, newlen);
		HPRINT("SYSCALL realloc(0x%lX:%zu, %zu) = 0x%lX\n",
			(long)src, (size_t)srclen, (size_t)newlen, (long)data);
		// When data != src, srclen is the old length, and the
		// chunks are non-overlapping, so we can use forwards memcpy.
		if (data != src && srclen != 0) {
			const char* src_ptr = machine.memory.template memarray<char>(src, srclen);
			machine.memory.copy_to_guest(data, src_ptr, std::min(address_t(srclen), newlen));
		}
		machine.set_result(data);
	});
	// Free n+3
	install_syscall_handler(syscall_base+3,
	[] (Machine& machine)
	{
		const auto ptr = machine.sysarg(0);
		if (ptr != 0x0)
		{
			[[maybe_unused]] int ret = machine.arena().free(ptr);
			HPRINT("SYSCALL free(0x%lX) = %d\n", (long)ptr, ret);
			//machine.set_result(ret);
			if (ret < 0) {
				throw MachineException(ILLEGAL_OPERATION, "Possible double-free for freed pointer", ptr);
			}
				return;
		}
		HPRINT("SYSCALL free(0x0) = 0\n");
		//machine.set_result(0);
		return;
	});
	// Meminfo n+4
	install_syscall_handler(syscall_base+4,
	[] (Machine& machine)
	{
		const auto dst = machine.sysarg(0);
		const auto& arena = machine.arena();
		struct Result {
			const address_t bf;
			const address_t bu;
			const address_t cu;
		} result = {
			.bf = (address_t) arena.bytes_free(),
			.bu = (address_t) arena.bytes_used(),
			.cu = (address_t) arena.chunks_used()
		};
		int ret = (dst != 0) ? 0 : -1;
		HPRINT("SYSCALL meminfo(0x%lX) = %d\n", (long)dst, ret);
		if (ret == 0) {
			machine.memory.copy_to_guest(dst, &result, sizeof(result));
		}
		machine.set_result(ret);
	});

	// Patch malloc/free/calloc/realloc symbols
	patch(*this, {"malloc"},    syscall_base+0);
	patch(*this, {"calloc"},    syscall_base+1);
	patch(*this, {"realloc"},   syscall_base+2);
	patch(*this, {"free"},      syscall_base+3);
}

const Arena& Machine::arena() const
{
	if (!this->has_arena()) {
		throw MachineException(FEATURE_DISABLED, "Native-performance heap not enabled");
	}
	return *this->m_arena;
}
Arena& Machine::arena()
{
	if (!this->has_arena()) {
		throw MachineException(FEATURE_DISABLED, "Native-performance heap not enabled");
	}
	return *this->m_arena;
}

} // loongarch
