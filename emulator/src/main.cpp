#include <libloong/machine.hpp>
#include <libloong/threaded_bytecodes.hpp>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <inttypes.h>
#include <memory>

#ifndef _WIN32
#include <getopt.h>
#define HAVE_GETOPT_LONG
#endif

using namespace loongarch;

struct EmulatorOptions {
	std::string binary_path;
	std::vector<std::string> program_args;
	uint64_t max_instructions = 0; // unlimited
	uint64_t memory_max = 2048ull << 20; // 2 GB
	bool verbose = false;
	bool precise = false;
	bool timing = false;
	bool silent = false;
	bool show_bytecode_stats = false;
};

// ELF class constants
static constexpr uint8_t ELFCLASS32 = 1;
static constexpr uint8_t ELFCLASS64 = 2;

static std::vector<uint8_t> load_file(const char* filename)
{
	std::ifstream file(filename, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		throw std::runtime_error(std::string("Failed to open file: ") + filename);
	}

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<uint8_t> buffer(size);
	if (!file.read((char*)buffer.data(), size)) {
		throw std::runtime_error("Failed to read file");
	}

	return buffer;
}

static void print_bytecode_statistics(const Machine& machine)
{
	printf("\n=== Bytecode Usage Statistics ===\n\n");

	auto stats = machine.collect_bytecode_statistics();
	if (stats.empty()) {
		printf("No bytecode statistics available (decoder cache not populated)\n");
		return;
	}

	// Calculate total instructions
	uint64_t total = 0;
	for (const auto& stat : stats) {
		total += stat.count;
	}

	printf("%-20s %12s %10s\n", "Bytecode", "Count", "Percentage");
	printf("%-20s %12s %10s\n", "--------", "-----", "----------");

	for (const auto& stat : stats) {
		const char* name = loongarch::bytecode_name(stat.bytecode);
		const double percentage = (100.0 * stat.count) / total;

		// For fallback bytecodes (FUNCTION), decode the sample instruction using the printer
		if (stat.bytecode == loongarch::LA64_BC_FUNCTION &&
		    stat.sample_instruction != 0) {
			// Decode the instruction to get its printer
			loongarch::la_instruction instr;
			instr.whole = stat.sample_instruction;
			const auto& decoded = loongarch::CPU::decode(instr);

			if (decoded.printer) {
				char buffer[256];
				// Most printers don't use the CPU parameter, but some might.
				// We create a temporary CPU on the stack for safety.
				try {
					// Call the printer - pass nullptr for CPU as most don't use it
					int printed = decoded.printer(buffer, sizeof(buffer),
						machine.cpu, instr, 0);

					if (printed > 0 && buffer[0] != '\0') {
						// Extract just the mnemonic (first word before space)
						std::string mnemonic(buffer);
						size_t space_pos = mnemonic.find(' ');
						if (space_pos != std::string::npos) {
							mnemonic = mnemonic.substr(0, space_pos);
						}

						printf("%-20s %12" PRIu64 " %9.2f%% (%s)\n",
							   name, stat.count, percentage, mnemonic.c_str());
					} else {
						// Printer returned nothing, show hex
						printf("%-20s %12" PRIu64 " %9.2f%% (0x%08x)\n",
							   name, stat.count, percentage, stat.sample_instruction);
					}
				} catch (...) {
					// Printer crashed (probably used CPU), show hex
					printf("%-20s %12" PRIu64 " %9.2f%% (0x%08x)\n",
						   name, stat.count, percentage, stat.sample_instruction);
				}
			} else {
				// No printer, show hex
				printf("%-20s %12" PRIu64 " %9.2f%% (0x%08x)\n",
					   name, stat.count, percentage, stat.sample_instruction);
			}
		} else {
			printf("%-20s %12" PRIu64 " %9.2f%%\n", name, stat.count, percentage);
		}
	}

	printf("\nTotal instructions in cache: %" PRIu64 "\n", total);
}

static int run_program(const std::vector<uint8_t>& binary, const EmulatorOptions& opts)
{
	std::unique_ptr<Machine> machine;
	try {
		// Create machine
		machine = std::make_unique<Machine>(binary, MachineOptions{
			.memory_max = opts.memory_max,
			.verbose_loader = opts.verbose,
			.verbose_syscalls = opts.verbose,
		});

		// Setup Linux syscalls
		machine->setup_linux_syscalls();
		// Setup accelerated syscalls
		machine->setup_accelerated_syscalls();

		// Setup program arguments
		if (opts.verbose) {
			printf("Arguments:\n");
			for (const auto& arg : opts.program_args) {
				printf("  %s\n", arg.c_str());
			}
		}
		machine->setup_linux(opts.program_args, {"LC_ALL=C", "USER=groot"});

		if (opts.verbose) {
			printf("Program entry point at: 0x%" PRIx64 "\n",
				   (uint64_t)machine->memory.start_address());
		}

		const auto t0 = std::chrono::high_resolution_clock::now();

		// Run the program
		if (opts.precise) {
			machine->set_max_instructions(opts.max_instructions);
			machine->set_instruction_counter(0);
			machine->cpu.simulate_precise();
		} else if (opts.max_instructions == 0) {
			machine->cpu.simulate_inaccurate(machine->cpu.pc());
		} else {
			machine->simulate(opts.max_instructions);
		}

		const auto t1 = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> elapsed = t1 - t0;

		// Show bytecode statistics if requested
		if (opts.show_bytecode_stats) {
			print_bytecode_statistics(*machine);
		}

		// Check if stopped normally
		if (!machine->instruction_limit_reached()) {
			const int exit_code = machine->template return_value<int>();
			if (!opts.silent) {
				if (opts.max_instructions != 0) {
					printf("Program exited with code %d after %" PRIu64 " instructions (%.3f seconds, %.2f MI/s)\n",
							exit_code,
							machine->instruction_counter(),
							elapsed.count(),
							(machine->instruction_counter() / (elapsed.count() * 1e6)));
				} else if (opts.timing) {
					printf("Program exited with code %d (%.3f seconds)\n",
							exit_code,
							elapsed.count());
				} else {
					printf("Program exited with code %d\n", exit_code);
				}
			}
			return exit_code;
		} else {
			if (!opts.silent) {
				fprintf(stderr, "Execution timeout after %" PRIu64 " instructions",
						machine->instruction_counter());
				if (opts.timing) {
					fprintf(stderr, " (%.6f seconds)", elapsed.count());
				}
				fprintf(stderr, "\n");
			}
			return -1;
		}

	} catch (const MachineException& e) {
		fprintf(stderr, "Machine exception: %s, data: 0x%llx (%ld)\n",
			e.what(), (unsigned long long)e.data(), (long)e.data());
		if (machine) {
			fprintf(stderr, "  Instruction count: %" PRIu64 "\n", machine->instruction_counter());
			fprintf(stderr, "%s\n", machine->cpu.registers().to_string().c_str());
		}
		return -1;
	} catch (const std::exception& e) {
		fprintf(stderr, "Error: %s\n", e.what());
		return -1;
	}
}

static void print_help(const char* progname)
{
	printf("Usage: %s [options] <program> [args...]\n\n", progname);
	printf("LoongArch Emulator - Execute LoongArch ELF binaries\n\n");
	printf("Options:\n");
	printf("  -h, --help              Show this help message\n");
	printf("  -v, --verbose           Enable verbose output (loader & syscalls)\n");
	printf("  -s, --silent            Suppress all output except errors\n");
	printf("      --precise           Use precise simulation mode (slower)\n");
	printf("  -t, --timing            Show execution timing and instruction count\n");
	printf("      --stats             Show bytecode usage statistics after execution\n");
	printf("  -f, --fuel <num>        Maximum instructions to execute (default: 2000000000)\n");
	printf("                          Use 0 for unlimited execution\n");
	printf("  -m, --memory <size>     Maximum memory in MiB (default: 512)\n\n");
	printf("The emulator automatically detects LA32/LA64 architecture from the ELF binary.\n\n");
	printf("Examples:\n");
	printf("  %s program.elf\n", progname);
	printf("  %s --verbose --timing program.elf arg1 arg2\n", progname);
	printf("  %s --stats --fuel 1000000 program.elf\n", progname);
	printf("  %s --fuel 1000000 --memory 256 program.elf\n\n", progname);
	printf("Check if fast-path differs from slow-path (precise):\n");
	printf("  %s --precise program.elf\n\n", progname);
}

static EmulatorOptions parse_arguments(int argc, char* argv[])
{
	EmulatorOptions opts;

#ifdef HAVE_GETOPT_LONG
	static const struct option long_options[] = {
		{"help",    no_argument,       0, 'h'},
		{"verbose", no_argument,       0, 'v'},
		{"precise", no_argument,       0, '\x03'},
		{"silent",  no_argument,       0, 's'},
		{"timing",  no_argument,       0, 't'},
		{"stats",   no_argument,       0, '\x02'},
		{"fuel",    required_argument, 0, 'f'},
		{"memory",  required_argument, 0, 'm'},
		{0, 0, 0, 0}
	};

	int opt;
	while ((opt = getopt_long(argc, argv, "hvstf:m:", long_options, nullptr)) != -1) {
		switch (opt) {
		case 'h':
			print_help(argv[0]);
			exit(0);
		case 'v':
			opts.verbose = true;
			break;
		case 's':
			opts.silent = true;
			break;
		case 't':
			opts.timing = true;
			break;
		case 'f':
			if (strcasecmp(optarg, "max") == 0) {
				opts.max_instructions = UINT64_MAX;
			} else {
				opts.max_instructions = strtoull(optarg, nullptr, 10);
			}
			break;
		case 'm':
			opts.memory_max = strtoull(optarg, nullptr, 10) << 20; // Convert MiB to bytes
			break;
		case '\x02':
			opts.show_bytecode_stats = true;
			break;
		case '\x03':
			opts.precise = true;
			break;
		default:
			print_help(argv[0]);
			exit(1);
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "Error: No program file specified\n\n");
		print_help(argv[0]);
		exit(1);
	}

	opts.binary_path = argv[optind];
	// Collect program arguments (including the program name as argv[0])
	for (int i = optind; i < argc; i++) {
		opts.program_args.push_back(argv[i]);
	}
#else
	// Fallback for systems without getopt_long (e.g., Windows)
	// Support environment variables for configuration
	if (getenv("VERBOSE") != nullptr)
		opts.verbose = true;
	if (getenv("SILENT") != nullptr)
		opts.silent = true;
	if (getenv("TIMING") != nullptr)
		opts.timing = true;
	if (getenv("STATS") != nullptr)
		opts.show_bytecode_stats = true;
	if (getenv("FUEL") != nullptr) {
		opts.max_instructions = strtoull(getenv("FUEL"), nullptr, 10);
		if (opts.max_instructions == 0) {
			opts.max_instructions = UINT64_MAX;
		}
	}
	if (getenv("MEMORY") != nullptr)
		opts.memory_max = strtoull(getenv("MEMORY"), nullptr, 10) << 20;

	// Simple argument parsing
	int first_non_option = 1;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			print_help(argv[0]);
			exit(0);
		} else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
			opts.verbose = true;
			first_non_option++;
		} else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--silent") == 0) {
			opts.silent = true;
			first_non_option++;
		} else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timing") == 0) {
			opts.timing = true;
			first_non_option++;
		} else if (strcmp(argv[i], "--stats") == 0) {
			opts.show_bytecode_stats = true;
			first_non_option++;
		} else {
			break; // First non-option argument
		}
	}

	if (first_non_option >= argc) {
		fprintf(stderr, "Error: No program file specified\n\n");
		print_help(argv[0]);
		exit(1);
	}

	opts.binary_path = argv[first_non_option];
	// Collect program arguments
	for (int i = first_non_option; i < argc; i++) {
		opts.program_args.push_back(argv[i]);
	}
#endif

	return opts;
}

int main(int argc, char* argv[])
{
	EmulatorOptions opts = parse_arguments(argc, argv);

	try {
		// Load binary
		auto binary = load_file(opts.binary_path.c_str());

		if (binary.size() < 5) {
			fprintf(stderr, "Error: File too small to be a valid ELF binary\n");
			return 1;
		}

		// Check ELF magic number
		if (binary[0] != 0x7f || binary[1] != 'E' || binary[2] != 'L' || binary[3] != 'F') {
			fprintf(stderr, "Error: Not a valid ELF binary\n");
			return 1;
		}

		// Detect architecture from ELF class (byte 4)
		const uint8_t elf_class = binary[4];
		const bool is_64bit = (elf_class == ELFCLASS64);
		const bool is_32bit = (elf_class == ELFCLASS32);

		if (!is_32bit && !is_64bit) {
			fprintf(stderr, "Error: Unknown ELF class: %d\n", elf_class);
			return 1;
		}

		if (opts.verbose) {
			fprintf(stderr, "Loaded %zu bytes from %s\n", binary.size(), opts.binary_path.c_str());
			fprintf(stderr, "Detected %s architecture\n", is_64bit ? "LA64" : "LA32");
		}

		// Run program with detected architecture
		if (is_32bit) {
			fprintf(stderr, "Error: 32-bit LoongArch is not supported!\n");
		} else { // is_64bit
			return run_program(binary, opts);
		}

	} catch (const std::exception& e) {
		fprintf(stderr, "Fatal error: %s\n", e.what());
		return 1;
	}
}
