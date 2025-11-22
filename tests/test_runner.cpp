#include <libloong/machine.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <cassert>
using namespace loongarch;
static constexpr uint64_t MAX_INSTRUCTIONS = 2'000'000'000ull;
static constexpr uint64_t MAX_MEMORY = 512 * 1024 * 1024; // 512 MB

// Read file into memory
std::vector<uint8_t> read_file(const std::string& path) {
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file) {
		throw std::runtime_error("Failed to open file: " + path);
	}

	auto size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<uint8_t> buffer(size);
	if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
		throw std::runtime_error("Failed to read file: " + path);
	}

	return buffer;
}

// Test harness for running LoongArch programs
struct TestResult {
	std::string name;
	bool success;
	int exit_code;
	std::string error;
	uint64_t instructions_executed;
	std::string output;  // Captured stdout/stderr
	bool reached_main = false;
	address_type<LA64> final_pc = 0;
};

TestResult run_test(const std::string& name, const std::string& path, int expected_exit_code) {
	TestResult result;
	result.name = name;
	result.success = false;
	result.exit_code = -1;
	result.instructions_executed = 0;

	try {
		// Load binary
		auto binary = read_file(path);
		std::string_view binary_view(reinterpret_cast<const char*>(binary.data()), binary.size());

		// Create machine options
		MachineOptions<LA64> options;
		options.verbose_loader = false;
		options.memory_max = MAX_MEMORY;

		// Create machine
		Machine<LA64> machine(binary_view, options);

		// Setup minimal environment
		machine.setup_linux_syscalls();
		machine.setup_linux({"program"}, {"LC_ALL=C", "USER=groot"});

		// Run the program with a reasonable instruction limit
		try {
			machine.simulate(MAX_INSTRUCTIONS);
		} catch (const MachineException& e) {
			// Program hit an error during execution
			result.error = std::string("Exception: ") + e.what();
		}


		// Get exit code from register A0 (only if machine stopped cleanly)
		result.final_pc = machine.cpu.pc();

		// Check if main was reached (look up the symbol)
		auto main_addr = machine.address_of("main");
		if (main_addr != 0) {
			// Check if we passed through main during execution
			// For now, just record if main exists
			result.reached_main = (result.final_pc >= main_addr &&
			                       result.final_pc <= main_addr + 0x1000);
		}

		if (machine.stopped()) {
			result.exit_code = machine.cpu.reg(REG_A0);
			result.success = (result.exit_code == expected_exit_code);
			if (!result.success && result.error.empty()) {
				result.error = "Exit code mismatch: expected " + std::to_string(expected_exit_code) +
				               ", got " + std::to_string(result.exit_code);
			}
		} else if (result.error.empty()) {
			result.exit_code = -1;
			result.error = "Program did not exit cleanly (PC=0x" +
			               std::to_string(result.final_pc) + ")";
		}
		result.instructions_executed = machine.instruction_counter();

	} catch (const std::exception& e) {
		result.error = std::string("Exception: ") + e.what();
	}

	return result;
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <test_dir>" << std::endl;
		return 1;
	}

	std::string test_dir = argv[1];
	std::cout << "Running libloong tests from: " << test_dir << std::endl << std::endl;

	// Define tests: {name, file, expected_exit_code}
	struct TestCase {
		std::string name;
		std::string file;
		int expected_exit;
	};

	std::vector<TestCase> tests = {
		{"Return 42 (bare)", "return_42_bare", 42},
		{"Return 42", "return_42", 42},
		{"Simple Add", "simple_add", 42},
		{"Hello World", "hello_world", 0},
	};

	int passed = 0;
	int failed = 0;

	std::vector<TestResult> results;

	for (const auto& test : tests) {
		std::string path = test_dir + "/" + test.file;

		if (!std::filesystem::exists(path)) {
			std::cout << "[ SKIP ] " << test.name << " (binary not found)" << std::endl;
			continue;
		}

		std::cout << "[  RUN ] " << test.name << std::endl;
		auto result = run_test(test.name, path, test.expected_exit);
		results.push_back(result);

		if (result.success) {
			std::cout << "[   OK ] " << test.name
			          << " (exit=" << result.exit_code
			          << ", insns=" << result.instructions_executed << ")" << std::endl;
			passed++;
		} else {
			std::cout << "[ FAIL ] " << test.name << ": " << result.error << std::endl;
			failed++;
		}
	}

	// Print detailed summary
	std::cout << std::endl;
	std::cout << "===============================================" << std::endl;
	std::cout << "Tests passed: " << passed << "/" << (passed + failed) << std::endl;
	std::cout << "===============================================" << std::endl;
	std::cout << std::endl;

	// Print details for failed tests
	if (failed > 0) {
		std::cout << "Failed test details:" << std::endl;
		for (const auto& result : results) {
			if (!result.success) {
				std::cout << "  " << result.name << ":" << std::endl;
				std::cout << "    Error: " << result.error << std::endl;
				std::cout << "    Instructions: " << result.instructions_executed << std::endl;
				std::cout << std::hex << "    Final PC: 0x" << result.final_pc << std::dec << std::endl;
				std::cout << std::endl;
			}
		}
	}

	return (failed == 0) ? 0 : 1;
}
