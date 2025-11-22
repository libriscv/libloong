#pragma once
#include <cstdint>

namespace loongarch
{
	// ELF file support for LoongArch
	struct Elf {
		// ELF header
		struct Header {
			uint8_t  ident[16];
			uint16_t type;
			uint16_t machine;
			uint32_t version;
			uint64_t entry;
			uint64_t phoff;
			uint64_t shoff;
			uint32_t flags;
			uint16_t ehsize;
			uint16_t phentsize;
			uint16_t phnum;
			uint16_t shentsize;
			uint16_t shnum;
			uint16_t shstrndx;
		};

		// Program header
		struct ProgramHeader {
			uint32_t type;
			uint32_t flags;
			uint64_t offset;
			uint64_t vaddr;
			uint64_t paddr;
			uint64_t filesz;
			uint64_t memsz;
			uint64_t align;
		};

		// Section header
		struct SectionHeader {
			uint32_t name;
			uint32_t type;
			uint64_t flags;
			uint64_t addr;
			uint64_t offset;
			uint64_t size;
			uint32_t link;
			uint32_t info;
			uint64_t addralign;
			uint64_t entsize;
		};

	// Symbol
	struct Sym {
		uint32_t name;
		uint8_t  info;
		uint8_t  other;
		uint16_t shndx;
		uint64_t value;
		uint64_t size;
	};

	// Dynamic section entry
	struct Dynamic {
		int64_t tag;
		uint64_t val;
	};

	// Relocation with addend
	struct Rela {
		uint64_t offset;
		uint64_t info;
		int64_t addend;
	};

	// LoongArch machine types
	static constexpr uint16_t EM_LOONGARCH = 258;		// Program header types
		static constexpr uint32_t PT_LOAD = 1;
		static constexpr uint32_t PT_DYNAMIC = 2;
		static constexpr uint32_t PT_INTERP = 3;
		static constexpr uint32_t PT_TLS = 7;

	// Program header flags
	static constexpr uint32_t PF_X = 1;
	static constexpr uint32_t PF_W = 2;
	static constexpr uint32_t PF_R = 4;

	// Section header types
	static constexpr uint32_t SHT_NULL = 0;
	static constexpr uint32_t SHT_SYMTAB = 2;
	static constexpr uint32_t SHT_STRTAB = 3;
	static constexpr uint32_t SHT_DYNSYM = 11;

	// Symbol binding
	static constexpr uint8_t STB_LOCAL = 0;
	static constexpr uint8_t STB_GLOBAL = 1;
	static constexpr uint8_t STB_WEAK = 2;

	// Symbol type
	static constexpr uint8_t STT_NOTYPE = 0;
	static constexpr uint8_t STT_OBJECT = 1;
	static constexpr uint8_t STT_FUNC = 2;
	static constexpr uint8_t STT_SECTION = 3;
	static constexpr uint8_t STT_FILE = 4;

	// Dynamic section tags
	static constexpr int64_t DT_NULL = 0;
	static constexpr int64_t DT_RELA = 7;
	static constexpr int64_t DT_RELASZ = 8;

	static constexpr uint8_t ST_BIND(uint8_t info) { return info >> 4; }
	static constexpr uint8_t ST_TYPE(uint8_t info) { return info & 0xf; }
};

template <int W>
struct Symbol {
	address_type<W> address;
	address_type<W> size;
	std::string name;
};

} // loongarch
