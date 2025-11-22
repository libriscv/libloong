#include <libloong/machine.hpp>
#include <iostream>
#include <vector>
#include <cstring>

using namespace loongarch;

int main()
{
	std::cout << "libloong Simple Example\n";
	std::cout << "=======================\n\n";

	// Create a simple binary with a few instructions
	// This would normally be loaded from an ELF file
	// For demonstration, we'll create a minimal program manually

	// Note: In real usage, you would load an actual ELF binary
	std::vector<uint8_t> fake_elf(4096, 0);

	// Minimal ELF header (not a real ELF, just for demo)
	memcpy(fake_elf.data(), "\x7f\x45\x4c\x46", 4); // ELF magic

	try {
		// Create a 64-bit LoongArch machine with 16MB memory
		Machine<LA64> machine { fake_elf, {
			.memory_max = 16 * 1024 * 1024,
			.verbose_loader = true
		}};

		std::cout << "Machine created successfully!\n";
		std::cout << "Entry point: 0x" << std::hex << machine.memory.start_address() << std::dec << "\n";
		std::cout << "Stack pointer: 0x" << std::hex << machine.cpu.reg(REG_SP) << std::dec << "\n";

		// Install Linux syscalls
		machine.setup_linux_syscalls();

		// Setup program arguments
		std::vector<std::string> args = {"example_program", "arg1", "arg2"};
		machine.setup_linux(args, {});

		// Display initial register state
		std::cout << "\nInitial state:\n";
		std::cout << "  PC: 0x" << std::hex << machine.cpu.pc() << std::dec << "\n";
		std::cout << "  SP: 0x" << std::hex << machine.cpu.reg(REG_SP) << std::dec << "\n";

		// Memory access example
		std::cout << "\nMemory operations:\n";
		uint64_t test_addr = machine.cpu.reg(REG_SP);
		machine.memory.write<uint64_t>(test_addr, 0xDEADBEEFCAFEBABE);
		uint64_t value = machine.memory.read<uint64_t>(test_addr);
		std::cout << "  Wrote and read back: 0x" << std::hex << value << std::dec << "\n";

		std::cout << "\nExample completed successfully!\n";

		return 0;

	} catch (const MachineException& e) {
		std::cerr << "Machine exception: " << e.what() << "\n";
		std::cerr << "  Type: " << e.type() << "\n";
		std::cerr << "  Data: 0x" << std::hex << e.data() << std::dec << "\n";
		return 1;
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << "\n";
		return 1;
	}
}
