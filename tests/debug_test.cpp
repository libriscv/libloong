#include <libloong/debug.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdlib>

#ifndef _WIN32
#include <getopt.h>
#define HAVE_GETOPT_LONG
#endif

using namespace loongarch;
static std::unique_ptr<Machine> machine;

struct DebugOptions {
	std::string binary_path;
	uint64_t max_instructions = 10'000'000ull;
	size_t memory_max = 256 * 1024 * 1024; // 256 MB
	bool verbose_loader = true;
	bool verbose_syscalls = true;
	bool verbose_registers = false;
	bool compare_objdump = false;
	bool short_output = false;
	std::string call_function;
	std::vector<std::string> arguments;
	bool value_is_expected = false;
	long expected_value = 0; // Used by --call
};

static void print_help(const char* progname)
{
	std::cout << "Usage: " << progname << " [options] <binary>\n\n"
		<< "LoongArch Debugger - Trace and debug LoongArch binaries\n\n"
		<< "Options:\n"
		<< "  -h, --help              Show this help message\n"
		<< "  -i, --max-instructions  Maximum instructions to execute (default: 10000000)\n"
		<< "  -m, --memory            Maximum memory in MiB (default: 256)\n"
		<< "  -q, --quiet             Disable verbose loader and syscalls\n"
		<< "  -r, --registers         Show register state after each instruction\n"
		<< "  -o, --compare-objdump   Compare with objdump and stop on mnemonic mismatch\n"
		<< "  -s, --short             Use short output format\n"
		<< "  -c, --call <function>   Call a function after init (and debug that)\n"
		<< "      --arg <value>       Appends argument to pass to function call\n"
		<< "      --expect <value>    Optional expected return value from call\n\n"
		<< "Examples:\n"
		<< "  " << progname << " program.elf\n"
		<< "  " << progname << " --max-instructions 1000000 --short program.elf\n"
		<< "  " << progname << " -q -r program.elf\n"
		<< "  " << progname << " -o program.elf  # Stop if instruction mismatch detected\n\n"
		<< "  " << progname << " --call test42 --expect 42 program.elf\n";
}

static std::vector<uint8_t> read_file(const std::string& path)
{
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

static DebugOptions parse_arguments(int argc, char* argv[])
{
	DebugOptions opts;

#ifdef HAVE_GETOPT_LONG
	static const struct option long_options[] = {
		{"help",             no_argument,       0, 'h'},
		{"max-instructions", required_argument, 0, 'i'},
		{"memory",           required_argument, 0, 'm'},
		{"quiet",            no_argument,       0, 'q'},
		{"registers",        no_argument,       0, 'r'},
		{"compare-objdump",  no_argument,       0, 'o'},
		{"short",            no_argument,       0, 's'},
		{"call",             required_argument, 0, 'c'},
		{"arg",              required_argument, 0, '\x02'},
		{"expect",           required_argument, 0, '\x01'},
		{0, 0, 0, 0}
	};

	int opt;
	while ((opt = getopt_long(argc, argv, "hi:m:qrosc:", long_options, nullptr)) != -1) {
		switch (opt) {
		case 'h':
			print_help(argv[0]);
			exit(0);
		case 'i':
			opts.max_instructions = strtoull(optarg, nullptr, 10);
			if (opts.max_instructions == 0) {
				opts.max_instructions = UINT64_MAX;
			}
			break;
		case 'm':
			opts.memory_max = strtoull(optarg, nullptr, 10) << 20; // Convert MiB to bytes
			break;
		case 'q':
			opts.verbose_loader = false;
			opts.verbose_syscalls = false;
			break;
		case 'r':
			opts.verbose_registers = true;
			break;
		case 'o':
			opts.compare_objdump = true;
			break;
		case 's':
			opts.short_output = true;
			break;
		case 'c':
			opts.call_function = optarg;
			break;
		case '\x01':
			opts.value_is_expected = true;
			opts.expected_value = strtol(optarg, nullptr, 10);
			break;
		case '\x02':
			opts.arguments.push_back(optarg);
			break;
		default:
			print_help(argv[0]);
			exit(1);
		}
	}

	if (optind >= argc) {
		std::cerr << "Error: No binary file specified\n\n";
		print_help(argv[0]);
		exit(1);
	}

	opts.binary_path = argv[optind];
#else
	// Fallback for systems without getopt_long (e.g., Windows)
	// Support environment variables for configuration
	if (getenv("COMPARE_OBJDUMP") != nullptr)
		opts.compare_objdump = true;
	if (getenv("VERBOSE_REGISTERS") != nullptr)
		opts.verbose_registers = (strcmp(getenv("VERBOSE_REGISTERS"), "1") == 0);
	if (getenv("SHORT") != nullptr)
		opts.short_output = true;
	if (getenv("QUIET") != nullptr) {
		opts.verbose_loader = false;
		opts.verbose_syscalls = false;
	}
	if (getenv("MAX_INSTRUCTIONS") != nullptr)
		opts.max_instructions = strtoull(getenv("MAX_INSTRUCTIONS"), nullptr, 10);
	if (getenv("MEMORY_MAX") != nullptr)
		opts.memory_max = strtoull(getenv("MEMORY_MAX"), nullptr, 10) << 20;

	// Simple argument parsing
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			print_help(argv[0]);
			exit(0);
		}
	}

	if (argc < 2) {
		std::cerr << "Error: No binary file specified\n\n";
		print_help(argv[0]);
		exit(1);
	}

	opts.binary_path = argv[argc - 1];
#endif

	return opts;
}

int main(int argc, char* argv[])
{
	DebugOptions opts = parse_arguments(argc, argv);

	try {
		// Load binary
		const std::vector<uint8_t> binary = read_file(opts.binary_path);
		std::string_view binary_view(reinterpret_cast<const char*>(binary.data()), binary.size());

		// Create machine options
		MachineOptions options;
		options.memory_max = opts.memory_max;
		options.verbose_loader = opts.verbose_loader;
		options.verbose_syscalls = opts.verbose_syscalls;

		// Create machine
		machine = std::make_unique<Machine>(binary_view, options);

		// Setup minimal environment with program name
		machine->setup_linux_syscalls();
		machine->setup_linux({"program"}, {"LC_TYPE=C", "LC_ALL=C", "USER=groot"});

		// Create debug wrapper
		DebugMachine debug_machine(*machine);
		debug_machine.filename = opts.binary_path;
		debug_machine.compare_objdump = opts.compare_objdump;
		debug_machine.stop_on_objdump_mismatch = opts.compare_objdump; // Automatically stop on mismatch when comparing
		debug_machine.verbose_registers = opts.verbose_registers;
		debug_machine.short_output = opts.short_output;

		if (opts.call_function.empty()) {
			std::cout << "* Starting execution at PC=0x" << std::hex
				<< machine->cpu.pc() << std::dec << std::endl;
			std::cout << std::endl;

			// Execute with instruction tracing until program exits
			debug_machine.simulate(opts.max_instructions);
		} else {
			machine->simulate(10'000'000ull); // Run initial setup

			auto func_addr = machine->address_of(opts.call_function);
			if (func_addr == 0) {
				throw std::runtime_error("Function not found: " + opts.call_function);
			}

			std::cout << "* Calling '" << opts.call_function
				<< "' at 0x" << std::hex << func_addr << std::dec
				<< " with arguments:";
			for (const auto& arg : opts.arguments) {
				std::cout << " '" << arg << "'";
			}
			std::cout << std::endl;

			long result = debug_machine.vmcall(func_addr, opts.max_instructions, opts.arguments);
			if (opts.value_is_expected && machine->stopped()) {
				if (result == opts.expected_value) {
					std::cout << "* Function returned expected value: " << result << std::endl;
					return 0;
				} else {
					std::cerr << "* Function returned " << result
						<< ", expected " << opts.expected_value << std::endl;
					return 1;
				}
			} else {
				std::cout << "* Function returned value: " << result << std::endl;
			}
		}

		if (machine->stopped()) {
			const int exit_code = static_cast<int>(machine->cpu.reg(REG_A0));
			std::cout << "Program exited with code: " << exit_code << std::endl;
		} else {
			std::cout << "Execution stopped. Final state:" << std::endl;
			debug_machine.print_instruction();
			debug_machine.print_registers();
		}

	} catch (const MachineException& me) {
		std::cerr << "MachineException: " << me.what()
			<< " (type=" << static_cast<int>(me.type())
			<< ", data=0x" << std::hex << me.data() << std::dec << ")" << std::endl;
		if (machine) {
			DebugMachine debug_machine(*machine);
			debug_machine.print_registers();
		}
		return 1;
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
