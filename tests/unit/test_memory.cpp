#include <catch2/catch_test_macros.hpp>
#include "codebuilder.hpp"
#include "test_utils.hpp"
#include <cstring>
#include <vector>

using namespace loongarch;
using namespace loongarch::test;

TEST_CASE("Guest memory range checks", "[memory]") {
	CodeBuilder builder;
	const auto binary = builder.build(R"(
		int main() {
			return 0;
		}
	)", "memory_ranges");

	TestMachine tm(binary);
	auto& mem = tm.machine().memory;
	const address_t arena_end = mem.arena_size();
	const address_t stack = mem.stack_address() - 4096;
	std::vector<uint8_t> buffer(8192);

	SECTION("Valid accesses") {
		REQUIRE_NOTHROW(mem.copy_to_guest(stack, buffer.data(), 64));
		REQUIRE_NOTHROW(mem.copy_from_guest(buffer.data(), stack, 64));
		REQUIRE_NOTHROW(mem.memset(stack, 0, 64));
		REQUIRE_NOTHROW(mem.memarray<uint64_t>(stack, 8));
		REQUIRE_NOTHROW(mem.writable_memarray<uint64_t>(stack, 8));
		REQUIRE_NOTHROW(mem.copy_from_guest(buffer.data(), arena_end - 64, 64));
	}

	SECTION("Empty arrays are not range-checked") {
		REQUIRE(mem.memarray<uint8_t>(~address_t(0), 0) != nullptr);
		REQUIRE(mem.writable_memarray<uint8_t>(0, 0) != nullptr);
	}

	SECTION("Length extending past the end of the arena") {
		REQUIRE_THROWS(mem.copy_to_guest(arena_end - 8, buffer.data(), buffer.size()));
		REQUIRE_THROWS(mem.copy_from_guest(buffer.data(), arena_end - 8, buffer.size()));
		REQUIRE_THROWS(mem.memset(arena_end - 8, 0, buffer.size()));
		REQUIRE_THROWS(mem.memarray<uint8_t>(arena_end - 8, buffer.size()));
		REQUIRE_THROWS(mem.writable_memarray<uint8_t>(arena_end - 8, buffer.size()));
		REQUIRE_THROWS(mem.memview(arena_end - 8, buffer.size()));
		REQUIRE_THROWS(mem.memcmp(stack, arena_end - 8, buffer.size()));
		REQUIRE_THROWS(mem.copy_into_arena_unsafe(arena_end - 8, buffer.data(), buffer.size()));
	}

	SECTION("Address plus length wrapping around") {
		const address_t wrap = ~address_t(0) - 7;
		REQUIRE_THROWS(mem.copy_to_guest(wrap, buffer.data(), 16));
		REQUIRE_THROWS(mem.copy_from_guest(buffer.data(), wrap, 16));
		REQUIRE_THROWS(mem.memcmp(wrap, stack, 16));
		REQUIRE_THROWS(mem.copy_into_arena_unsafe(wrap, buffer.data(), 16));
		REQUIRE_THROWS(mem.strlen(wrap));
	}

	SECTION("Element count overflowing the byte length") {
		const size_t count = (~size_t(0) / sizeof(uint64_t)) + 2;
		REQUIRE_THROWS(mem.memarray<uint64_t>(stack, count));
		REQUIRE_THROWS(mem.writable_memarray<uint64_t>(stack, count));
	}

	SECTION("Misaligned arrays") {
		REQUIRE_THROWS(mem.memarray<uint32_t>(stack + 1, 1));
		REQUIRE_THROWS(mem.writable_memarray<uint64_t>(stack + 4, 1));
	}

	SECTION("strlen is limited to the arena") {
		mem.memset(arena_end - 16, 'A', 16);
		REQUIRE(mem.strlen(arena_end - 16, 4096) == 16);
		REQUIRE_THROWS(mem.strlen(arena_end));
	}
}
