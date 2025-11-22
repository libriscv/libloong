#pragma once
#include "common.hpp"
#include "page.hpp"
#include "decoded_exec_segment.hpp"
#include "elf.hpp"
#include <vector>
#include <memory>
#include <string_view>
#include <unordered_map>

namespace loongarch
{
	template <int W> struct Symbol;

	template <int W>
	struct alignas(LA_MACHINE_ALIGNMENT) Memory
	{
		using address_t = address_type<W>;

		Memory(Machine<W>& machine, std::string_view binary, const MachineOptions<W>& options);
		Memory(Machine<W>& machine, const Machine<W>& other, const MachineOptions<W>& options);
		~Memory();

		// Memory access
		template <typename T>
		T read(address_t addr) const;

		template <typename T>
		void write(address_t addr, T value);

		// Memory arena operations
		void copy_to_guest(address_t dest, const void* src, size_t len);
		void copy_from_guest(void* dest, address_t src, size_t len) const;
		template <typename T>
		const T* memarray(address_t addr, size_t count = 1) const; // Read-only view
		template <typename T>
		T* writable_memarray(address_t addr, size_t count = 1); // Read-write view
		void memset(address_t addr, uint8_t value, size_t len);
		int memcmp(address_t addr1, address_t addr2, size_t len) const;
		size_t strlen(address_t addr, size_t maxlen = 4096);
		std::string memstring(address_t addr, size_t maxlen = 4096);

		// Memory mapping
		address_t mmap_allocate(size_t size);
		void mmap_deallocate(address_t addr, size_t size);

		// Execute segments
		DecodedExecuteSegment<W>& create_execute_segment(
			const MachineOptions<W>& options,
			const void* data, address_t addr, size_t len,
			bool is_initial, bool is_likely_jit = false);

		std::shared_ptr<DecodedExecuteSegment<W>> exec_segment_for(address_t pc) const;
		size_t execute_segments_count() const noexcept { return m_exec.size() + (m_main_exec_segment ? 1 : 0); }
		void evict_execute_segments();

		// Binary info
		const auto& binary() const noexcept { return m_binary; }
		address_t start_address() const noexcept { return m_start_address; }
		address_t stack_address() const noexcept { return m_stack_address; }
		void set_stack_address(address_t addr) noexcept { m_stack_address = addr; }
		address_t exit_address() const noexcept { return m_exit_address; }
		void set_exit_address(address_t addr) noexcept { m_exit_address = addr; }

		// Arena boundaries
		address_t rodata_start() const noexcept { return m_rodata_start; }
		address_t data_start() const noexcept { return m_data_start; }
		address_t arena_size() const noexcept { return m_arena_size; }
		void allocate_custom_arena(size_t size, address_t rodata_start, address_t data_start);

		// Heap (brk) management
		address_t heap_address() const noexcept { return m_heap_address; }
		void set_heap_address(address_t addr) noexcept { m_heap_address = addr; }
		address_t brk_address() const noexcept { return m_brk_address; }
		void set_brk_address(address_t addr) noexcept { m_brk_address = addr; }
		address_t mmap_address() const noexcept { return m_mmap_address; }

		// Statistics
		size_t memory_usage_counter() const noexcept { return m_arena_size; }

		// Machine reference
		Machine<W>& machine() noexcept { return m_machine; }
		const Machine<W>& machine() const noexcept { return m_machine; }

		// Symbol lookup
		address_t address_of(const std::string& name) const;
		const Symbol<W>* lookup_symbol(address_t addr) const;

		// ELF information for auxv
		address_t elf_phdr_addr() const noexcept { return m_elf_phdr_addr; }
		uint16_t elf_phentsize() const noexcept { return m_elf_phentsize; }
		uint16_t elf_phnum() const noexcept { return m_elf_phnum; }

		void reset();

	private:
		Machine<W>& m_machine;

		// Single memory arena (mmap'd on POSIX, new[] otherwise)
		uint8_t* m_arena = nullptr;
		size_t m_arena_size = 0;

		// Memory region boundaries
		address_t m_rodata_start = 0;  // Start of read-only data
		address_t m_data_start = 0;    // Start of writable data/heap

		std::string_view m_binary; // Non-owning reference to binary data

		// Execute segments
		std::shared_ptr<DecodedExecuteSegment<W>> m_main_exec_segment;
		std::vector<std::shared_ptr<DecodedExecuteSegment<W>>> m_exec;

		// Memory layout
		address_t m_start_address = 0;
		address_t m_stack_address = 0;
		address_t m_exit_address = 0;
		address_t m_heap_address = 0;
		address_t m_brk_address = 0;
		address_t m_mmap_address = 0;

		// ELF header information for auxv
		address_t m_elf_phdr_addr = 0;
		uint16_t m_elf_phentsize = 0;
		uint16_t m_elf_phnum = 0;

		// ELF loader
		void binary_loader(const MachineOptions<W>& options);
		void parse_symbols(const Elf::Header* ehdr, const MachineOptions<W>& options);
		void parse_symbol_table(const Elf::SectionHeader* symtab,
		                        const Elf::SectionHeader* strtab,
		                        const MachineOptions<W>& options);
		void process_relocations(const Elf::Header* ehdr, const MachineOptions<W>& options);
		void process_rela_section(size_t offset, size_t size, const MachineOptions<W>& options);

		// Symbol storage
		std::vector<Symbol<W>> m_symbols;

		// Arena helpers
		void allocate_arena(size_t size);
		void free_arena();
		inline bool is_valid_address(address_t addr) const noexcept {
			return addr >= m_rodata_start && addr < m_arena_size;
		}
		inline bool is_writable(address_t addr, size_t size = sizeof(address_t)) const noexcept {
			return addr >= m_data_start && addr + size < m_arena_size;
		}
	};

} // loongarch

#include "memory_inline.hpp"
