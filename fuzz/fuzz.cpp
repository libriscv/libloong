#include <libloong/machine.hpp>
#include <vector>
#include <cstdint>
#include <cstdlib>
using namespace loongarch;
static constexpr uint32_t MAX_INSTRUCTIONS = 5'000;

// In order to be able to inspect a coredump we want to
// crash on every ASAN error.
extern "C" void __asan_on_error()
{
	abort();
}
extern "C" void __msan_on_error()
{
	abort();
}

/**
 * Fuzzing the instruction set only is very fast, and sometimes enough if
 * new instructions were added, and no other parts of the code has been
 * touched.
 *
 * This function takes random data, rounds it up to 4-byte alignment,
 * creates a fast-path execute segment using init_execute_area, and
 * executes it with a limited instruction count.
**/
static void fuzz_instruction_set(const uint8_t* data, size_t len)
{
	// Round length down to 4-byte boundary (LoongArch instructions are 4 bytes)
	const size_t aligned_len = (len / 4) * 4;
	if (aligned_len == 0)
		return;

	constexpr uint32_t EXEC_ADDR  =  0x10000;
	constexpr uint32_t STACK_ADDR = 0x800000;

	try
	{
		// Create machine with custom arena
		Machine machine { std::string_view{}, {} };
		machine.memory.allocate_custom_arena(16ull << 20, 0x10000, 0x20000);

		// Initialize stack pointer
		machine.cpu.reg(REG_SP) = STACK_ADDR;

		// Create executable area from fuzzer input data
		// The data is already byte-aligned, init_execute_area expects it rounded to 4 bytes
		machine.cpu.init_execute_area(data, EXEC_ADDR, aligned_len);

		// Jump to the execute area and start execution
		machine.cpu.jump(EXEC_ADDR);

		// Let's avoid infinite loops by limiting instruction count
		machine.simulate(MAX_INSTRUCTIONS);
	}
	catch (const std::exception &e)
	{
		// Silently catch exceptions during fuzzing
		// Uncomment for debugging:
		// printf(">>> Exception: %s\n", e.what());
	}
}

static void fuzz_elf_binary(const uint8_t* data, size_t len)
{
	try
	{
		// Create machine from ELF binary in fuzzer input data
		Machine machine { std::string_view{reinterpret_cast<const char*>(data), len}, {} };

		// Start execution at entry point
		machine.cpu.jump(machine.memory.start_address());

		// Let's avoid infinite loops by limiting instruction count
		machine.simulate(MAX_INSTRUCTIONS);
	}
	catch (const std::exception &e)
	{
		// Silently catch exceptions during fuzzing
		// Uncomment for debugging:
		// printf(">>> Exception: %s\n", e.what());
	}
}

/**
 * LibFuzzer entry point
 * This function is called by LibFuzzer for each test input
**/
extern "C"
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t len)
{
#if FUZZER == 1
	fuzz_instruction_set(data, len);
#elif FUZZER == 2
	fuzz_elf_binary(data, len);
#else
	#error "FUZZER macro not defined or invalid"
#endif
	return 0;
}
