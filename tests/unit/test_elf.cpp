#include <catch2/catch_test_macros.hpp>
#include "codebuilder.hpp"
#include "test_utils.hpp"
#include <cstring>

using namespace loongarch;
using namespace loongarch::test;

static Elf::SectionHeader* section_header(std::vector<uint8_t>& bin, size_t idx) {
	auto* ehdr = reinterpret_cast<Elf::Header*>(bin.data());
	return reinterpret_cast<Elf::SectionHeader*>(bin.data() + ehdr->shoff) + idx;
}
static Elf::SectionHeader* find_section(std::vector<uint8_t>& bin, const char* name) {
	auto* ehdr = reinterpret_cast<Elf::Header*>(bin.data());
	const auto* shstrtab = section_header(bin, ehdr->shstrndx);
	for (size_t i = 0; i < ehdr->shnum; i++) {
		auto* shdr = section_header(bin, i);
		if (strcmp((const char*)bin.data() + shstrtab->offset + shdr->name, name) == 0)
			return shdr;
	}
	return nullptr;
}

TEST_CASE("Malformed ELF section headers", "[elf]") {
	CodeBuilder builder;
	const auto binary = builder.build(R"(
		int main() {
			return 42;
		}
	)", "elf_hardening");

	SECTION("Well-formed ELF resolves symbols") {
		TestMachine machine(binary);
		REQUIRE(machine.machine().address_of("main") != 0);
	}

	SECTION(".text size that wraps around the address space") {
		auto bin = binary;
		auto* text = find_section(bin, ".text");
		REQUIRE(text != nullptr);
		text->size = ~uint64_t(0) - 0xFF;
		TestMachine machine(bin);
		machine.setup_linux();
		auto result = machine.execute();
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}

	SECTION(".symtab offset that wraps around the address space") {
		auto bin = binary;
		auto* symtab = find_section(bin, ".symtab");
		REQUIRE(symtab != nullptr);
		symtab->offset = ~uint64_t(0) - 0xF;
		TestMachine machine(bin);
		REQUIRE(machine.machine().address_of("main") == 0);
	}

	SECTION(".strtab without NUL-termination at end of file") {
		auto bin = binary;
		const uint64_t offset = bin.size();
		bin.resize(bin.size() + 16, 'A');
		auto* strtab = find_section(bin, ".strtab");
		REQUIRE(strtab != nullptr);
		strtab->offset = offset;
		strtab->size = 16;
		TestMachine machine(bin);
		REQUIRE(machine.machine().address_of("main") == 0);
	}

	SECTION("Out-of-bounds symtab link") {
		auto bin = binary;
		auto* symtab = find_section(bin, ".symtab");
		REQUIRE(symtab != nullptr);
		symtab->link = 0xFFFF;
		TestMachine machine(bin);
		REQUIRE(machine.machine().address_of("main") == 0);
	}

	SECTION("Section header table outside the binary") {
		auto bin = binary;
		reinterpret_cast<Elf::Header*>(bin.data())->shoff = ~uint64_t(0) - 0x40;
		TestMachine machine(bin);
		REQUIRE(machine.machine().address_of("main") == 0);
	}
}
