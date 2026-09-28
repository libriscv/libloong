#include "machine.hpp"

#include "elf.hpp"
#include <cstring>
#include <algorithm>

namespace loongarch {

struct TextSegmentBounds {
	address_t start = 0;
	address_t size = 0;
	bool found = false;
};

static TextSegmentBounds find_text_section(const Memory& mem, const Elf::Header* ehdr)
{
	TextSegmentBounds bounds;

	// Need section headers to find executable sections
	const auto* shdrs = mem.elf_section_headers(ehdr);
	if (shdrs == nullptr) {
		return bounds;
	}

	// Span all executable sections (.plt, .iplt, .init, .text, .fini, ...)
	static constexpr uint64_t SHF_EXECINSTR = 0x4;
	address_t lo = ~address_t(0), hi = 0;
	for (size_t i = 0; i < ehdr->shnum; i++) {
		const auto* shdr = &shdrs[i];
		if (!(shdr->flags & SHF_EXECINSTR) || shdr->size == 0 || shdr->addr == 0) {
			continue;
		}
		const address_t end = shdr->addr + shdr->size;
		if (end < shdr->addr) {
			return bounds; // Malformed: fall back to the whole segment
		}
		lo = std::min(lo, address_t(shdr->addr));
		hi = std::max(hi, end);
	}
	lo &= ~address_t(3); // Align start down to instruction boundary
	if (hi > lo) {
		bounds.start = lo;
		bounds.size = hi - lo;
		bounds.found = true;
	}
	return bounds;
}

void Memory::binary_loader(const MachineOptions& options)
{
	if (this->m_binary.size() < sizeof(Elf::Header)) {
		throw MachineException(INVALID_PROGRAM, "Binary too small");
	}

	const Elf::Header* ehdr = reinterpret_cast<const Elf::Header*>(this->m_binary.data());
	if (ehdr->ident[0] != 0x7f || ehdr->ident[1] != 'E' ||
		ehdr->ident[2] != 'L' || ehdr->ident[3] != 'F') {
		throw MachineException(INVALID_PROGRAM, "Not an ELF file");
	}
	if (ehdr->machine != Elf::EM_LOONGARCH) {
		throw MachineException(INVALID_PROGRAM, "Not a LoongArch ELF file");
	}
	this->m_start_address = ehdr->entry;

	// Store ELF header info for auxiliary vector
	this->m_elf_phentsize = sizeof(Elf::ProgramHeader);
	this->m_elf_phnum = ehdr->phnum;
	// phdr address will be set after we know the base address
	if (ehdr->phnum == 0 || ehdr->phentsize != sizeof(Elf::ProgramHeader) || ehdr->phnum > 256) {
		throw MachineException(INVALID_PROGRAM, "Invalid program headers in ELF file");
	}
	const address_t elf_phdr_end_address = ehdr->phoff + ehdr->phnum * sizeof(Elf::ProgramHeader);
	if (elf_phdr_end_address > this->m_binary.size() || elf_phdr_end_address < ehdr->phoff) {
		throw MachineException(INVALID_PROGRAM, "Program headers invalid");
	}

	// Find memory bounds
	address_t min_addr = ~address_t(0);
	address_t max_addr = 0;
	address_t first_writable = ~address_t(0);

	for (size_t i = 0; i < ehdr->phnum; i++) {
		const Elf::ProgramHeader* phdr = reinterpret_cast<const Elf::ProgramHeader*>(
		this->m_binary.data() + ehdr->phoff + i * sizeof(Elf::ProgramHeader));
		if (phdr->type == Elf::PT_LOAD) {
			address_t start = phdr->vaddr;
			address_t end = phdr->vaddr + phdr->memsz;
			if (end > options.memory_max || end < start) {
				throw MachineException(INVALID_PROGRAM, "ELF segment invalid", phdr->vaddr);
			}
			min_addr = std::min(min_addr, start);
			max_addr = std::max(max_addr, end);
			if ((phdr->flags & Elf::PF_W) && start < first_writable) {
				first_writable = start;
			}
		}
	}
	if (min_addr >= max_addr) {
		throw MachineException(INVALID_PROGRAM, "No loadable segments found");
	}

	// Page align max_addr - this is where heap begins
	max_addr = (max_addr + 4095) & ~address_t(4095);

	this->m_rodata_start = min_addr;
	this->m_data_start = (first_writable != ~address_t(0)) ? first_writable : max_addr;

	// Set phdr address - it's at base + phoff
	// For statically linked binaries, phdr is at the file offset from the base
	this->m_elf_phdr_addr = min_addr + ehdr->phoff;

	// Initialize heap at end of loaded data
	this->m_heap_address = max_addr;
	// mmap starts at end of brk and grows upward
	this->m_mmap_address = this->m_heap_address;
	// Allocate BRK area (initially zero size)
	this->m_brk_address = this->mmap_allocate(options.brk_size);
	// Allocate stack from mmap region (grows downward from top)
	// m_stack_address is the TOP of the stack (highest address)
	const address_t stack_base = this->mmap_allocate(options.stack_size);
	this->m_stack_address = stack_base + options.stack_size;

	if (this->m_heap_address >= options.memory_max) {
		if (options.verbose_loader) {
			fprintf(stderr, "Error: Not enough memory for stack and brk:\n");
			fprintf(stderr, "  heap_begin: 0x%lx\n", (unsigned long)this->m_heap_address);
			fprintf(stderr, "  memory_max: 0x%lx\n", (unsigned long)options.memory_max);
		}
		throw MachineException(OUT_OF_MEMORY,
			"Not enough memory for stack and brk",
			static_cast<uint64_t>(options.memory_max));
	}

	if (options.custom_arena_pointer != nullptr &&
		options.custom_arena_size >= options.memory_max) {
		this->use_custom_arena(options.custom_arena_pointer, options.custom_arena_size);
	} else {
		this->allocate_arena(options.memory_max);
	}

	if (options.verbose_loader) {
		const size_t arena_size = options.memory_max - this->m_heap_address;
		fprintf(stderr, "Memory layout:\n");
		fprintf(stderr, "  min_addr: 0x%lx\n", (unsigned long)min_addr);
		fprintf(stderr, "  max_addr (heap_begin): 0x%lx\n", (unsigned long)max_addr);
		fprintf(stderr, "  rodata start: 0x%lx\n", (unsigned long)m_rodata_start);
		fprintf(stderr, "  data start: 0x%lx\n", (unsigned long)m_data_start);
		fprintf(stderr, "  heap address: 0x%lx\n", (unsigned long)m_heap_address);
		fprintf(stderr, "  mmap address: 0x%lx\n", (unsigned long)m_mmap_address);
		fprintf(stderr, "  stack begin: 0x%lx end: 0x%lx\n",
			(unsigned long)stack_base,
			(unsigned long)(stack_base + options.stack_size));
		fprintf(stderr, "  memory size: 0x%lx (%lu MiB)\n",
			(unsigned long)arena_size,
			(unsigned long)arena_size / (1024 * 1024));
	}

	// Find .text section bounds to limit execute segment creation
	const TextSegmentBounds text_bounds = find_text_section(*this, ehdr);

	// Load segments into memory
	for (size_t i = 0; i < ehdr->phnum; i++) {
		const auto* phdr = reinterpret_cast<const Elf::ProgramHeader*>(
			m_binary.data() + ehdr->phoff + i * sizeof(Elf::ProgramHeader));

		if (phdr->type == Elf::PT_LOAD && phdr->filesz > 0) {
			const size_t offset = phdr->vaddr;
			const address_t end_vaddr = offset + phdr->filesz;
			if (end_vaddr > m_arena_size || end_vaddr < offset) {
				throw MachineException(INVALID_PROGRAM, "ELF segment invalid", phdr->vaddr);
			}
			const address_t file_end = offset + phdr->filesz;
			if (file_end > m_arena_size || file_end < offset) {
				throw MachineException(INVALID_PROGRAM, "ELF segment invalid", phdr->vaddr);
			}
			if (phdr->offset + phdr->filesz > m_binary.size() ||
				phdr->offset + phdr->filesz < phdr->offset) {
				throw MachineException(INVALID_PROGRAM, "ELF segment invalid", phdr->vaddr);
			}
			std::memcpy(m_arena + offset, m_binary.data() + phdr->offset, phdr->filesz);
			// Execute segment creation
			if (phdr->flags & Elf::PF_X) {
				address_t exec_vaddr = phdr->vaddr;
				size_t exec_size = phdr->filesz & ~size_t(3);
				size_t file_offset = phdr->offset;

				// Use .text section bounds if available to avoid translating data
				if (text_bounds.found) {
					// Check if .text section is within this segment
					const address_t seg_start = phdr->vaddr;
					const address_t seg_end = phdr->vaddr + phdr->filesz;

					if (text_bounds.start >= seg_start && text_bounds.size <= phdr->filesz
						&& text_bounds.start + text_bounds.size <= seg_end) {
						exec_vaddr = text_bounds.start;
						exec_size  = text_bounds.size & ~size_t(3); // Align to instruction boundary
						file_offset = phdr->offset + (text_bounds.start - seg_start);

						if (options.verbose_loader) {
							fprintf(stderr, "Creating execute segment for .text section: vaddr=0x%lx size=0x%lx\n",
								(unsigned long)exec_vaddr, (unsigned long)exec_size);
						}
					}
				}

				create_execute_segment(options,
					(const void*)(m_binary.data() + file_offset),
					exec_vaddr, exec_size, true, false);
			}
		}
	}

	// Parse symbols from section headers
	if (ehdr->shoff > 0 && ehdr->shnum > 0) {
		parse_symbols(ehdr, options);
	}
}

} // loongarch
