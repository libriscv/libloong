#include "benchmark.hpp"
#include <libloong/machine.hpp>
#include <fstream>
#include <memory>
#include <vector>

namespace benchmark {

using namespace loongarch;

// Global machine instance and binary
static std::unique_ptr<Machine> g_machine;
static std::vector<uint8_t> g_binary;
static uint64_t empty_addr = 0;
static uint64_t test_args0_addr = 0;

// Load the guest binary from file
static std::vector<uint8_t> load_binary(const std::string& path) {
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file) {
		throw std::runtime_error("Failed to open guest binary: " + path);
	}

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<uint8_t> buffer(size);
	if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
		throw std::runtime_error("Failed to read guest binary: " + path);
	}

	return buffer;
}

// Initialize the machine with the guest binary
static long syscall_counter = 0;
void initialize(const std::string& binary_path) {
	if (g_machine != nullptr) {
		return;
	}

	// Load binary
	g_binary = load_binary(binary_path);

	std::string_view binary_view(
		reinterpret_cast<const char*>(g_binary.data()),
		g_binary.size());

	// Create machine with reasonable options
	MachineOptions options;
#ifdef LA_BINARY_TRANSLATION
	options.translate_enabled = true;
	options.translate_automatic_nbit_address_space = true;
	options.translate_ignore_instruction_limit = true;
	options.translate_use_register_caching = true;
#endif

	g_machine = std::make_unique<Machine>(binary_view, options);

	// Setup Linux syscalls
	g_machine->setup_linux_syscalls();
	static constexpr uint32_t HEAP_SIZE = 32ull << 20; // 32 MB heap
	const auto heap_begin = g_machine->memory.mmap_allocate(HEAP_SIZE);
	g_machine->setup_accelerated_heap(heap_begin, HEAP_SIZE);
	g_machine->setup_linux({"benchmark_guest"}, {});

	Machine::install_syscall_handler(1, [](Machine&) {
		// Custom empty syscall for benchmarking
		syscall_counter++;
	});

	// Set up exit address for vmcalls
	auto exit_addr = g_machine->address_of("fast_exit");
	if (exit_addr == 0) {
		throw std::runtime_error("fast_exit function not found in guest binary");
	}
	g_machine->memory.set_exit_address(exit_addr);

	// Get address of empty function for benchmarking
	empty_addr = g_machine->address_of("empty_function");
	if (empty_addr == 0) {
		throw std::runtime_error("empty_function not found in guest binary");
	}

	// Get address of test_args_0 function for benchmarking
	test_args0_addr = g_machine->address_of("test_args_0");
	if (test_args0_addr == 0) {
		throw std::runtime_error("test_args_0 not found in guest binary");
	}

	auto saved_regs = g_machine->cpu.registers();
	g_machine->simulate(1'000'000ull);
	g_machine->cpu.registers() = saved_regs;
}

static void reset_counter() {
	syscall_counter = 0;
}

// Test: Empty function call (pure vmcall overhead)
void test_empty_function() {
	g_machine->vmcall(empty_addr);
}

// Test: Function with N arguments
template <int N>
void test_args();
// Test: Syscall with N arguments
template <int N>
void test_syscall();

template<> void test_args<0>() {
	g_machine->vmcall(test_args0_addr);
}

template<> void test_args<1>() {
	static uint64_t func_addr = g_machine->address_of("test_args_1");
	g_machine->vmcall(func_addr, 1);
}

template<> void test_args<2>() {
	static uint64_t func_addr = g_machine->address_of("test_args_2");
	g_machine->vmcall(func_addr, 1, 2);
}

template<> void test_args<3>() {
	static uint64_t func_addr = g_machine->address_of("test_args_3");
	g_machine->vmcall(func_addr, 1, 2, 3);
}

template<> void test_args<4>() {
	static uint64_t func_addr = g_machine->address_of("test_args_4");
	g_machine->vmcall(func_addr, 1, 2, 3, 4);
}

template<> void test_args<5>() {
	static uint64_t func_addr = g_machine->address_of("test_args_5");
	g_machine->vmcall(func_addr, 1, 2, 3, 4, 5);
}

template<> void test_args<6>() {
	static uint64_t func_addr = g_machine->address_of("test_args_6");
	g_machine->vmcall(func_addr, 1, 2, 3, 4, 5, 6);
}

template<> void test_args<7>() {
	static uint64_t func_addr = g_machine->address_of("test_args_7");
	g_machine->vmcall(func_addr, 1, 2, 3, 4, 5, 6, 7);
}

template<> void test_args<8>() {
	static uint64_t func_addr = g_machine->address_of("test_args_8");
	g_machine->vmcall(func_addr, 1, 2, 3, 4, 5, 6, 7, 8);
}

template<> void test_syscall<0>() {
	static uint64_t func_addr = g_machine->address_of("test_syscall_0");
	g_machine->vmcall(func_addr);
}

template<> void test_syscall<1>() {
	static uint64_t func_addr = g_machine->address_of("test_syscall_1");
	g_machine->vmcall(func_addr);
}

static void test_fibonacci() {
	static uint64_t func_addr = g_machine->address_of("test_fibonacci");
	g_machine->vmcall<int>(func_addr, 40); // Fibonacci(40)
}

// Run all benchmarks
void run_all_benchmarks(int samples) {
	const int iterations = 1000;

	printf("Running libloong vmcall benchmarks\n");
	printf("Configuration: %d samples × %d iterations per sample\n\n", samples, iterations);

	// Measure benchmark overhead
	int64_t overhead = measure_overhead<iterations>(samples);
	printf("Benchmark overhead: %ldns per iteration\n\n", overhead);

	// Run benchmarks
	printf("=== VMCall Overhead Tests ===\n");

	auto empty_result = run_benchmark<iterations>(
		"empty function",
		samples,
		reset_counter,
		test_empty_function,
		overhead
	);
	print_result(empty_result);

	printf("\n=== Argument Passing Overhead ===\n");

	auto args0 = run_benchmark<iterations>(
		"args=0",
		samples,
		reset_counter,
		test_args<0>,
		overhead
	);
	print_result(args0);

	// Use args0 as base vmcall overhead for future tests
	const int64_t base_vmcall_overhead = overhead + args0.median_ns;

	auto args1 = run_benchmark<iterations>(
		"args=1",
		samples,
		reset_counter,
		test_args<1>,
		overhead
	);
	print_result(args1);

	auto args2 = run_benchmark<iterations>(
		"args=2",
		samples,
		reset_counter,
		test_args<2>,
		overhead
	);
	print_result(args2);

	auto args3 = run_benchmark<iterations>(
		"args=3",
		samples,
		reset_counter,
		test_args<3>,
		overhead
	);
	print_result(args3);

	auto args4 = run_benchmark<iterations>(
		"args=4",
		samples,
		reset_counter,
		test_args<4>,
		overhead
	);
	print_result(args4);

	auto args5 = run_benchmark<iterations>(
		"args=5",
		samples,
		reset_counter,
		test_args<5>,
		overhead
	);
	print_result(args5);

	auto args6 = run_benchmark<iterations>(
		"args=6",
		samples,
		reset_counter,
		test_args<6>,
		overhead
	);
	print_result(args6);

	auto args7 = run_benchmark<iterations>(
		"args=7",
		samples,
		reset_counter,
		test_args<7>,
		overhead
	);
	print_result(args7);

	auto args8 = run_benchmark<iterations>(
		"args=8",
		samples,
		reset_counter,
		test_args<8>,
		overhead
	);
	print_result(args8);

	// After this point, use call overhead as baseline
	printf("\n=== Syscall Overhead ===\n");

	auto syscall0 = run_benchmark<iterations>(
		"syscall 0",
		samples,
		reset_counter,
		test_syscall<0>,
		base_vmcall_overhead
	);
	const long syscall_counter_end = syscall_counter;
	if (syscall_counter_end != 1+iterations) {
		fprintf(stderr, "Expected %d syscalls, but counted %ld\n",
			1+iterations, syscall_counter_end);
		throw std::runtime_error("Syscall count mismatch - some syscalls may not have been executed correctly");
	}
	print_result(syscall0);

	auto syscall1 = run_benchmark<iterations>(
		"syscall 1",
		samples,
		reset_counter,
		test_syscall<1>,
		base_vmcall_overhead
	);
	print_result(syscall1);

	printf("\n=== Compute ===\n");

	auto fibonacci_result = run_benchmark<iterations>(
		"fibonacci(40)",
		samples,
		reset_counter,
		test_fibonacci,
		base_vmcall_overhead
	);
	print_result(fibonacci_result);

	printf("\n");
	printf("Note: Tests after argument passing subtract base vmcall overhead (%ldns)\n", base_vmcall_overhead);
	printf("      p75, p90, and p99 represent the 75th, 90th, and 99th percentiles\n");
}

} // namespace benchmark
