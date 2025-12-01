#include "machine.hpp"
#include <cstring>
#include <mutex>
#include "native/heap.hpp"

namespace loongarch
{
	Machine::Machine(std::string_view binary, const MachineOptions& options)
		: cpu(*this), memory(*this, binary, options),
		  m_arena(nullptr)
	{
		m_options_ptr = &options;
		cpu.reset();  // Reset CPU after memory is loaded
		// Initialize all system call handlers to a throwing stub on first creation (thread-safe)
		static std::once_flag init_flag;
		std::call_once(init_flag, []() {
			for (auto& handler : m_syscall_handlers) {
				handler = [](Machine& m) {
					const int sysnum = static_cast<int>(m.cpu.reg(REG_A7));
					throw MachineException(UNIMPLEMENTED_SYSCALL,
						"Unimplemented system call", sysnum);
				};
			}
		});
	}

	Machine::Machine(const std::vector<uint8_t>& binary, const MachineOptions& options)
		: Machine(std::string_view(
			reinterpret_cast<const char*>(binary.data()), binary.size()), options)
	{
	}

	Machine::~Machine()
	{
	}

	void Machine::setup_linux(
		const std::vector<std::string>& args,
		const std::vector<std::string>& env)
	{
		if (args.empty()) {
			throw MachineException(INVALID_PROGRAM,
				"At least one argument to setup_linux() (program name) is required");
		}
		// Auxiliary vector types
		constexpr address_t AT_NULL = 0;
		constexpr address_t AT_PHDR = 3;
		constexpr address_t AT_PHENT = 4;
		constexpr address_t AT_PHNUM = 5;
		constexpr address_t AT_PAGESZ = 6;
		constexpr address_t AT_BASE = 7;
		constexpr address_t AT_ENTRY = 9;
		constexpr address_t AT_UID = 11;
		constexpr address_t AT_EUID = 12;
		constexpr address_t AT_GID = 13;
		constexpr address_t AT_EGID = 14;
		constexpr address_t AT_HWCAP = 16;
		constexpr address_t AT_CLKTCK = 17;
		constexpr address_t AT_RANDOM = 25;

		// Setup argv and environ on stack
		address_t sp = cpu.reg(REG_SP);
		sp &= ~address_t(15);

		// Push environment variables
		std::vector<address_t> env_ptrs;
		for (const auto& e : env) {
			sp -= e.size() + 1;
			memory.copy_to_guest(sp, e.data(), e.size() + 1); // Zero-terminated
			env_ptrs.push_back(sp);
		}

		// Push arguments
		std::vector<address_t> arg_ptrs;
		for (const auto& arg : args) {
			sp -= arg.size() + 1;
			memory.copy_to_guest(sp, arg.data(), arg.size() + 1); // Zero-terminated
			arg_ptrs.push_back(sp);
		}
		// Re-align after variable-length strings
		sp &= ~address_t(15);

		// Reserve space for AT_RANDOM (16 bytes of random data)
		sp -= 16;
		const address_t random_addr = sp;
		std::array<uint8_t, 16> random_data;
		for (size_t i = 0; i < random_data.size(); i++) {
			random_data[i] = static_cast<uint8_t>(rand() % 256);
		}
		memory.copy_to_guest(random_addr, random_data.data(), random_data.size());

		// Build auxiliary vector (in reverse order since we push from high to low)
		const address_t at_base = memory.start_address() & ~address_t(0xFFFFFFLL);
		std::vector<std::pair<address_t, address_t>> auxv;
		auxv.push_back({AT_BASE, at_base});
		auxv.push_back({AT_RANDOM, random_addr});
		auxv.push_back({AT_CLKTCK, 100});           // Clock ticks per second
		auxv.push_back({AT_HWCAP, 0});              // Hardware capabilities
		auxv.push_back({AT_EGID, 1000});
		auxv.push_back({AT_GID, 1000});
		auxv.push_back({AT_EUID, 1000});
		auxv.push_back({AT_UID, 1000});
		auxv.push_back({AT_ENTRY, memory.start_address()});
		auxv.push_back({AT_PAGESZ, 4096});          // Page size
		auxv.push_back({AT_PHNUM, static_cast<address_t>(memory.elf_phnum())});
		auxv.push_back({AT_PHENT, static_cast<address_t>(memory.elf_phentsize())});
		auxv.push_back({AT_PHDR, memory.elf_phdr_addr()});
		auxv.push_back({AT_NULL, 0});

		// Push auxiliary vector (already in reverse order)
		sp -= auxv.size() * 2 * sizeof(address_t);
		memory.copy_to_guest(sp, auxv.data(), auxv.size() * 2 * sizeof(address_t));

		// Push NULL (end of envp)
		sp -= sizeof(address_t);
		memory.template write<address_t>(sp, 0);

		// Push envp pointers
		sp -= env_ptrs.size() * sizeof(address_t);
		memory.copy_to_guest(sp, env_ptrs.data(), env_ptrs.size() * sizeof(address_t));

		// Push NULL (end of argv)
		sp -= sizeof(address_t);
		memory.template write<address_t>(sp, 0);

		// Push argv pointers
		sp -= arg_ptrs.size() * sizeof(address_t);
		memory.copy_to_guest(sp, arg_ptrs.data(), arg_ptrs.size() * sizeof(address_t));

		// Push argc
		sp -= sizeof(address_t);
		memory.template write<address_t>(sp, args.size());

		// Update stack pointer
		cpu.reg(REG_SP) = sp;
	}

	address_t Machine::address_of(const std::string& name) const
	{
		return memory.address_of(name);
	}

	const Symbol* Machine::lookup_symbol(address_t addr) const
	{
		return memory.lookup_symbol(addr);
	}

	void Machine::print(const char* data, size_t len)
	{
		fwrite(data, 1, len, stdout);
	}

	void Machine::print(std::string_view str)
	{
		print(str.data(), str.size());
	}

	size_t Machine::serialize_to(std::vector<uint8_t>& vec) const
	{
		// Serialization not yet implemented
		(void)vec;
		return 0;
	}

	int Machine::deserialize_from(const std::vector<uint8_t>& vec)
	{
		// Deserialization not yet implemented
		(void)vec;
		return -1;
	}

} // loongarch
