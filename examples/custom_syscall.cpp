#include <libloong/machine.hpp>
#include <iostream>
#include <string_view>
#include <cstring>
using namespace loongarch;

// Custom syscall handler: print a debug message
static constexpr int SYSCALL_EXIT = 93;
static void syscall_exit(Machine<LA64>& machine)
{
	// Get exit code from a0
	auto exit_code = machine.cpu.reg(REG_A0);
	std::cout << "Guest program exited with code: " << exit_code << "\n";
	machine.stop();
}

int main()
{
	std::cout << "libloong Custom Syscall Example\n";
	std::cout << "================================\n\n";

	try {
		Machine<LA64> machine { std::string_view{}, {}};
		machine.memory.allocate_custom_arena(16ull << 20, 0x10000, 0x20000);
		machine.cpu.reg(REG_SP) = 0x800000; // Initialize stack pointer

		// Create an executable area
		const std::vector<uint32_t> instructions {
			0x02802004,  // li.w $a0, 8
			0x0281740b,  // li.w $a7, 93
			0x002b0000   // syscall 0x0
		};
		machine.cpu.init_execute_area(
			instructions.data(),
			0x1000,
			instructions.size() * sizeof(uint32_t)
		);

		// Install our custom syscalls
		machine.install_syscall_handler(SYSCALL_EXIT, syscall_exit);

		std::cout << "Custom syscalls installed:\n";
		std::cout << "  Syscall " << SYSCALL_EXIT << ": exit(code)\n";
		std::cout << std::endl;

		// Start execution at the address of our executable area
		machine.cpu.jump(0x1000);
		machine.simulate();
		return 0;

	} catch (const MachineException& e) {
		std::cerr << "Machine exception: " << e.what() << "\n";
		return 1;
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << "\n";
		return 1;
	}
}
