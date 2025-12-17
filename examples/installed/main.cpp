#include <libloong/machine.hpp>
#include <iostream>
#include <filesystem>
#include <fstream>

using namespace loongarch;

int main(int argc, char* argv[])
{
	std::cout << "libloong Installed Package Example\n";
	std::cout << "====================================\n\n";

	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <loongarch64_elf_file>\n";
		std::cerr << "Example: " << argv[0] << " /usr/share/libloong/tests/return_42_bare.elf\n";
		return 1;
	}

	const std::string elf_path = argv[1];

	// Check if the file exists
	if (!std::filesystem::exists(elf_path)) {
		std::cerr << "Error: File not found: " << elf_path << "\n";
		return 1;
	}

	try {
		// Read the ELF file
		std::ifstream file(elf_path, std::ios::binary);
		if (!file) {
			std::cerr << "Error: Cannot open file: " << elf_path << "\n";
			return 1;
		}

		// Read file into vector
		std::vector<uint8_t> elf_data(
			(std::istreambuf_iterator<char>(file)),
			std::istreambuf_iterator<char>()
		);

		std::cout << "Loaded ELF file: " << elf_path << "\n";
		std::cout << "Size: " << elf_data.size() << " bytes\n\n";

		// Create a 64-bit LoongArch machine with 16MB memory
		Machine machine { elf_data, {
			.memory_max = 16 * 1024 * 1024,
			.verbose_loader = false
		}};

		std::cout << "Machine created successfully!\n";
		std::cout << "Entry point: 0x" << std::hex << machine.memory.start_address() << std::dec << "\n";

		// Install Linux syscalls
		machine.setup_linux_syscalls();

		// Setup program arguments
		std::vector<std::string> args = {elf_path};
		for (int i = 2; i < argc; i++) {
			args.push_back(argv[i]);
		}
		machine.setup_linux(args, {});

		std::cout << "\nExecuting program...\n";

		// Run the program (for up to 100 million instructions)
		machine.simulate(100'000'000ull);

		std::cout << "\nProgram completed!\n";
		std::cout << "Exit code: " << machine.return_value() << "\n";
		std::cout << "Instructions executed: " << machine.instruction_counter() << "\n";

		return 0;

	} catch (const MachineException& e) {
		std::cerr << "\nMachine exception: " << e.what() << "\n";
		std::cerr << "  Type: " << e.type() << "\n";
		std::cerr << "  Data: 0x" << std::hex << e.data() << std::dec << "\n";
		return 1;
	} catch (const std::exception& e) {
		std::cerr << "\nError: " << e.what() << "\n";
		return 1;
	}
}
