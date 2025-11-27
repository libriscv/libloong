#include "machine.hpp"
#include "threaded_bytecodes.hpp"
#include <cstdio>
#include <inttypes.h>

namespace loongarch {

static constexpr int SYS_native_memcpy  = 508;
static constexpr int SYS_native_memset  = 509;
static constexpr int SYS_native_memcmp  = 510;
static constexpr int SYS_native_memmove = 511;
// ... perhaps more in the future
static constexpr bool VERBOSE_PATCHING = false;

template <int W>
static inline void patch(Machine<W>& machine, const std::vector<std::string> symbols, const int syscall_number)
{
	for (const auto& sym : symbols) {
		const auto addr = machine.address_of(sym);
		if (addr != 0) {
			if constexpr (VERBOSE_PATCHING) {
				printf("Patching accelerated syscall for %s at 0x%016" PRIx64 "\n",
					sym.c_str(), static_cast<uint64_t>(addr));
			}
			// Patch the function prologue to invoke the syscall
			// using syscall.imm bytecode
			DecoderData<W> entry;
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

template <int W>
void Machine<W>::setup_accelerated_syscalls()
{
	// Register accelerated syscalls here
	// The goal is to:
	// 1. Detect public symbols like memcpy, __memcpy etc.
	// 2. Write a system call rigmarole into the current
	//    decoder cache overwriting ~2-4 instructions at the symbol address.
	// 3. When the syscall is invoked, it jumps into the emulator's
	//    accelerated syscall handler which performs the operation natively.

	install_syscall_handler(SYS_native_memcpy,
	[] (Machine<W>& machine) {
		const address_t dest = machine.cpu.reg(REG_A0);
		const address_t src  = machine.cpu.reg(REG_A1);
		const size_t n       = machine.cpu.reg(REG_A2);
		// Perform native memcpy
		uint8_t* dest_ptr = machine.memory.template writable_memarray<uint8_t>(dest, n);
		const uint8_t* src_ptr = machine.memory.template memarray<uint8_t>(src, n);
		std::memcpy(dest_ptr, src_ptr, n);

		machine.set_result(dest);
	});

	install_syscall_handler(SYS_native_memset,
	[] (Machine<W>& machine) {
		throw "";
		const address_t dest = machine.cpu.reg(REG_A0);
		const int value      = machine.cpu.reg(REG_A1);
		const size_t n       = machine.cpu.reg(REG_A2);
		// Perform native memset
		uint8_t* dest_ptr = machine.memory.template writable_memarray<uint8_t>(dest, n);
		std::memset(dest_ptr, value, n);

		machine.set_result(dest);
	});

	install_syscall_handler(SYS_native_memcmp,
	[] (Machine<W>& machine) {
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
	[] (Machine<W>& machine) {
		const address_t dest = machine.cpu.reg(REG_A0);
		const address_t src  = machine.cpu.reg(REG_A1);
		const size_t n       = machine.cpu.reg(REG_A2);
		// Perform native memmove
		uint8_t* dest_ptr = machine.memory.template writable_memarray<uint8_t>(dest, n);
		const uint8_t* src_ptr = machine.memory.template memarray<uint8_t>(src, n);
		std::memmove(dest_ptr, src_ptr, n);

		machine.set_result(dest);
	});

	// More accelerated syscalls can be added here

	// Iterate the symbol table and patch known functions
	patch<W>(*this, {"__memcpy_lsx", "__memcpy_lasx", "__memcpy_aligned"}, SYS_native_memcpy);
}

#ifdef LA_32
	template struct Machine<LA32>;
#endif
#ifdef LA_64
	template struct Machine<LA64>;
#endif
} // loongarch
