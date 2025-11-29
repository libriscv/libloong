#include "memory.hpp"

#include "machine.hpp"
#include "elf.hpp"
#include <cstring>
#include <algorithm>
#define OVER_ALLOCATE_SIZE 64 /* Avoid SIMD bounds-check */

#ifdef __unix__
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace loongarch
{
template <int W>
extern void populate_decoder_cache(DecodedExecuteSegment<W>& segment, address_type<W> exec_begin, const uint8_t* code, size_t code_size);

template <int W>
Memory<W>::Memory(Machine<W>& machine,
	std::string_view binary, const MachineOptions<W>& options)
	: m_machine(machine), m_binary(binary)
{
	if (!binary.empty()) {
		binary_loader(options);
	}
}

template <int W>
Memory<W>::Memory(Machine<W>& machine, const Machine<W>& other, const MachineOptions<W>& options)
	: m_machine(machine)
{
	(void)other; (void)options;
	throw MachineException(FEATURE_DISABLED, "Fork constructor not yet implemented");
}

template <int W>
Memory<W>::~Memory()
{
	free_arena();
}

template <int W>
void Memory<W>::allocate_arena(size_t size)
{
	if (this->m_arena) free_arena();
#ifdef __unix__
	this->m_arena = static_cast<uint8_t*>(mmap(nullptr, size + OVER_ALLOCATE_SIZE,
		PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
	if (this->m_arena == MAP_FAILED) {
		this->m_arena = nullptr;
		throw MachineException(OUT_OF_MEMORY, "Failed to allocate memory arena");
	}
#else
	try {
		this->m_arena = new uint8_t[size + OVER_ALLOCATE_SIZE]();
	} catch (const std::bad_alloc&) {
		this->m_arena = nullptr;
		throw MachineException(OUT_OF_MEMORY, "Failed to allocate memory arena");
	}
#endif
	this->m_arena_size = size;
}
template <int W>
void Memory<W>::allocate_custom_arena(size_t size, address_t rodata_start, address_t data_start)
{
	if (rodata_start >= size || data_start >= size || rodata_start > data_start) {
		throw MachineException(INVALID_PROGRAM, "Invalid custom arena boundaries");
	}
	// Reallocate arena if size differs
	if (this->m_arena != nullptr && this->m_arena_size != size) {
		if (this->m_arena) free_arena();
		this->allocate_arena(size);
	} else if (this->m_arena == nullptr) {
		this->allocate_arena(size);
	}
	this->m_rodata_start = rodata_start;
	this->m_data_start = data_start;
}

template <int W>
void Memory<W>::free_arena()
{
	if (!this->m_arena) return;
#ifdef __unix__
	munmap(this->m_arena, this->m_arena_size + OVER_ALLOCATE_SIZE);
#else
	delete[] this->m_arena;
#endif
	this->m_arena = nullptr;
	this->m_arena_size = 0;
}

template <int W>
void Memory<W>::parse_symbols(const Elf::Header* ehdr, const MachineOptions<W>& options)
{
	// Validate section header table
	if (ehdr->shoff + ehdr->shnum * sizeof(Elf::SectionHeader) > m_binary.size()) {
		if (options.verbose_loader) {
			fprintf(stderr, "Warning: Invalid section header table\n");
		}
		return;
	}

	// Find symbol table and string table sections
	const Elf::SectionHeader* symtab = nullptr;
	const Elf::SectionHeader* strtab = nullptr;
	const Elf::SectionHeader* dynsym = nullptr;
	const Elf::SectionHeader* dynstr = nullptr;

	for (size_t i = 0; i < ehdr->shnum; i++) {
		const auto* shdr = reinterpret_cast<const Elf::SectionHeader*>(
			m_binary.data() + ehdr->shoff + i * sizeof(Elf::SectionHeader));

		if (shdr->type == Elf::SHT_SYMTAB) {
			symtab = shdr;
			// String table is usually the linked section
			if (shdr->link < ehdr->shnum) {
				strtab = reinterpret_cast<const Elf::SectionHeader*>(
					m_binary.data() + ehdr->shoff + shdr->link * sizeof(Elf::SectionHeader));
			}
		} else if (shdr->type == Elf::SHT_DYNSYM) {
			dynsym = shdr;
			if (shdr->link < ehdr->shnum) {
				dynstr = reinterpret_cast<const Elf::SectionHeader*>(
					m_binary.data() + ehdr->shoff + shdr->link * sizeof(Elf::SectionHeader));
			}
		}
	}

	// Parse static symbol table
	if (symtab && strtab) {
		parse_symbol_table(symtab, strtab, options);
	}

	// Parse dynamic symbol table
	if (dynsym && dynstr) {
		parse_symbol_table(dynsym, dynstr, options);
	}
}

template <int W>
void Memory<W>::parse_symbol_table(const Elf::SectionHeader* symtab,
                                    const Elf::SectionHeader* strtab,
                                    const MachineOptions<W>& options)
{
	// Validate section offsets and sizes
	if (symtab->offset + symtab->size > m_binary.size() ||
	    strtab->offset + strtab->size > m_binary.size()) {
		if (options.verbose_loader) {
			fprintf(stderr, "Warning: Invalid symbol or string table section\n");
		}
		return;
	}

	const size_t num_symbols = symtab->size / sizeof(Elf::Sym);
	const auto* symbols = reinterpret_cast<const Elf::Sym*>(m_binary.data() + symtab->offset);
	const char* string_table = reinterpret_cast<const char*>(m_binary.data() + strtab->offset);

	for (size_t i = 0; i < num_symbols; i++) {
		const auto& sym = symbols[i];

		// Only add function symbols with non-zero addresses
		uint8_t type = Elf::ST_TYPE(sym.info);
		if ((type == Elf::STT_FUNC || type == Elf::STT_OBJECT) && sym.value != 0) {
			if (sym.name < strtab->size) {
				const char* name = string_table + sym.name;
				if (name[0] != '\0') {
					m_symbols.push_back({static_cast<address_t>(sym.value), static_cast<address_t>(sym.size), name});

					if (false && options.verbose_loader) {
						fprintf(stderr, "Symbol: 0x%lx %s (size=%lu)\n",
							(unsigned long)sym.value, name, (unsigned long)sym.size);
					}
				}
			}
		}
	}
}

template <int W>
void Memory<W>::process_relocations(const Elf::Header* ehdr, const MachineOptions<W>& options)
{
	// For static binaries, look for .rela.dyn section in section headers
	if (ehdr->shoff == 0 || ehdr->shnum == 0) {
		return;  // No section headers
	}

	// Find .rela.dyn section
	for (size_t i = 0; i < ehdr->shnum; i++) {
		const auto* shdr = reinterpret_cast<const Elf::SectionHeader*>(
			m_binary.data() + ehdr->shoff + i * sizeof(Elf::SectionHeader));

		// SHT_RELA = 4
		if (shdr->type == 4 && shdr->size > 0) {
			// Check if this is .rela.dyn by looking at the section name
			// For now, process all RELA sections
			process_rela_section(shdr->offset, shdr->size, options);
		}
	}
}

template <int W>
void Memory<W>::process_rela_section(size_t offset, size_t size, const MachineOptions<W>& options)
{
	const size_t num_entries = size / sizeof(Elf::Rela);
	auto* rela = const_cast<Elf::Rela*>(reinterpret_cast<const Elf::Rela*>(m_binary.data() + offset));

	for (size_t i = 0; i < num_entries; i++) {
		uint32_t type = rela[i].info & 0xFFFFFFFF;
		(void)type;

		// Static loader overwrites everything anyway
	}
}

template <int W>
size_t Memory<W>::strlen(address_t addr, size_t maxlen)
{
	const address_t end_addr = std::min(addr + maxlen, m_arena_size);
	if (end_addr <= addr) return 0;
	const address_t size = end_addr - addr;
	const char* ptr = memarray<char>(addr, size);
	return ::strnlen(ptr, size);
}

template <int W>
std::string Memory<W>::memstring(address_t addr, size_t maxlen)
{
	const size_t len = this->strlen(addr, maxlen);
	const char* ptr = memarray<char>(addr, len);
	return std::string(ptr, len);
}

template <int W>
typename Memory<W>::address_t Memory<W>::mmap_allocate(size_t size)
{
	size = (size + 4095) & ~size_t(4095); // Align to page size
	const address_t result = this->m_mmap_address;
	this->m_mmap_address += size;
	return result;
}

template <int W>
void Memory<W>::mmap_deallocate(address_t addr, size_t size)
{
	// Allow relaxation of mmap area by moving m_mmap_address back
	if (addr + size == this->m_mmap_address) {
		this->m_mmap_address = addr;
	}
}

template <int W>
DecodedExecuteSegment<W>& Memory<W>::create_execute_segment(
	const MachineOptions<W>& options, const void* data, address_t addr, size_t len,
	bool is_initial, bool is_likely_jit)
{
	(void)options; (void)is_likely_jit;
	if (len % 4 != 0) {
		throw MachineException(INVALID_PROGRAM, "Execute segment length is not 4-byte aligned");
	}

	auto segment = std::make_shared<DecodedExecuteSegment<W>>(addr, addr + len);

	populate_decoder_cache(*segment, addr, static_cast<const uint8_t*>(data), len);

	if (is_initial) {
		m_main_exec_segment = segment;
	} else {
		m_exec.push_back(segment);
	}

	return *segment;
}

template <int W>
std::shared_ptr<DecodedExecuteSegment<W>> Memory<W>::exec_segment_for(address_t pc) const
{
	if (m_main_exec_segment && m_main_exec_segment->is_within(pc)) {
		return m_main_exec_segment;
	}
	for (auto& seg : m_exec) {
		if (seg->is_within(pc)) return seg;
	}
	return nullptr;
}

template <int W>
void Memory<W>::evict_execute_segments()
{
	machine().cpu.set_execute_segment(*CPU<W>::empty_execute_segment());
	m_exec.clear();
	m_main_exec_segment.reset();
}

template <int W>
void Memory<W>::reset()
{
	if (m_arena) {
#ifdef MADV_DONTNEED
		madvise(m_arena, m_arena_size, MADV_DONTNEED);
#else
		std::memset(m_arena, 0, m_arena_size);
#endif
	}
	evict_execute_segments();
}

template <int W>
address_type<W> Memory<W>::address_of(const std::string& name) const
{
	for (const auto& sym : m_symbols) {
		if (sym.name == name) {
			return sym.address;
		}
	}
	return 0;
}

template <int W>
const Symbol<W>* Memory<W>::lookup_symbol(address_t addr) const
{
	const Symbol<W>* best_match = nullptr;
	for (const auto& sym : m_symbols) {
		// Check if address is within symbol range
		if (addr >= sym.address && addr < sym.address + sym.size) {
			return &sym;
		}
		// Also track the closest symbol before this address (for functions with unknown size)
		if (sym.address <= addr) {
			if (!best_match || sym.address > best_match->address) {
				best_match = &sym;
			}
		}
	}
	return best_match;
}

#ifdef LA_32
template struct Memory<LA32>;
#endif
#ifdef LA_64
template struct Memory<LA64>;
#endif

} // loongarch
