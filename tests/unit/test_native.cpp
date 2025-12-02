#include <catch2/catch_test_macros.hpp>
#include "codebuilder.hpp"
#include "test_utils.hpp"
#include <libloong/guest_datatypes.hpp>

using namespace loongarch;
using namespace loongarch::test;

// Helper to generate the fast_exit function for vmcalls
static std::string FAST_EXIT_FUNCTION = R"(
 asm(".pushsection .text\n"
	".global fast_exit\n"
	".type fast_exit, @function\n"
	"fast_exit:\n"
	"	li.w $a7, 94\n"
	"	syscall 0\n"
	".popsection\n");
)";

static const uint64_t MAX_INSTRUCTIONS = 10'000'000ul;

// Setup native system calls for testing
static void setup_native_system_calls(Machine& machine)
{
	// Syscall-backed heap
	constexpr size_t heap_size = 65536;
	auto heap = machine.memory.mmap_allocate(heap_size);
	machine.setup_accelerated_heap(heap, heap_size);
}

TEST_CASE("Native helper syscalls - basic operations", "[native]")
{
	CodeBuilder builder;

	CompilerOptions opts;
	opts.optimization = 2;
	auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
		#include <stdlib.h>
		#include <string.h>

		int main(int argc, char** argv)
		{
			const char *hello = (const char*)atol(argv[1]);
			if (strcmp(hello, "Hello World!") != 0) {
				return 1;
			}
			return 666;
		}
	)", "native_basic", opts);

	TestMachine test_machine(binary);
	Machine& machine = test_machine.machine();

	setup_native_system_calls(machine);

	// Allocate string on heap
	static const std::string hello = "Hello World!";
	auto addr = machine.arena().malloc(64);
	machine.memory.copy_to_guest(addr, hello.data(), hello.size()+1);

	// Pass string address to guest as main argument
	test_machine.setup_linux(
		{"native", std::to_string(addr)},
		{"LC_TYPE=C", "LC_ALL=C", "USER=root"});

	// Run simulation
	machine.simulate(MAX_INSTRUCTIONS);

	// Check exit code - the test passes if the program can print and exit properly
	REQUIRE(machine.return_value() == 666);
}

TEST_CASE("VM calls with std::string and std::vector", "[native][vmcall]")
{
	CodeBuilder builder;

	CompilerOptions opts;
	opts.optimization = 2;
	opts.extra_flags.push_back("-fno-exceptions");
	auto binary = builder.build_cpp(FAST_EXIT_FUNCTION + R"(
		#include <string>
		#include <vector>
		#include <cassert>

		void* operator new(size_t size) {
			return malloc(size);
		}
		void operator delete(void* ptr) {
			free(ptr);
		}
		void operator delete(void* ptr, size_t) {
			free(ptr);
		}

		extern "C" __attribute__((used, retain))
		void test(std::string& str,
			const std::vector<int>& ints,
			const std::vector<std::string>& strings)
		{
			std::string result = "Hello, " + str + "! Integers:";
			for (auto i : ints)
				result += " " + std::to_string(i);
			result += " Strings:";
			for (const auto& s : strings)
				result += " " + s;
			str = result;
		}

		struct Data {
			int a, b, c, d;
		};

		extern "C" __attribute__((used, retain))
		void test2(Data* data) {
			assert(data->a == 1);
			assert(data->b == 2);
			assert(data->c == 3);
			assert(data->d == 4);
			data->a = 5;
			data->b = 6;
			data->c = 7;
			data->d = 8;
		}

		extern "C" __attribute__((used, retain))
		int test3(std::vector<std::vector<int>>& vec) {
			assert(vec.size() == 2);
			assert(vec[0].size() == 3);
			assert(vec[1].size() == 2);
			assert(vec[0][0] == 1);
			assert(vec[0][1] == 2);
			assert(vec[0][2] == 3);
			assert(vec[1][0] == 4);
			assert(vec[1][1] == 5);

			vec.at(1).push_back(666);
			return 666;
		}

		int main() {
			return 666;
		}
	)", "vmcall_native", opts);

	TestMachine test_machine(binary);
	Machine& machine = test_machine.machine();

	setup_native_system_calls(machine);
	test_machine.setup_linux(
		{"vmcall"},
		{"LC_TYPE=C", "LC_ALL=C", "USER=root"});

	machine.simulate(MAX_INSTRUCTIONS);
	REQUIRE(machine.return_value<int>() == 666);

	// Track allocations before tests
	const unsigned allocs_before = machine.arena().allocation_counter() -
		machine.arena().deallocation_counter();

	SECTION("Test GuestStdString and GuestStdVector operations") {
		for (int i = 0; i < 10; i++) {
			// Create a GuestStdString object with a string
			ScopedCppString str(machine);
			REQUIRE(str->empty());
			str = "C++ World ..SSO..";
			REQUIRE(str->to_string(machine) == "C++ World ..SSO..");

			// Create a GuestStdVector object with a vector of integers
			ScopedCppVector<int> ivec(machine);
			REQUIRE(ivec->empty());
			ivec = std::vector<int>{ 1, 2, 3 };
			REQUIRE(ivec->size() == 3);
			ivec->assign(machine, std::vector<int>{ 1, 2, 3, 4, 5 });
			REQUIRE(ivec->size() == 5);

			// Create a vector of strings using a specialization for std::string
			ScopedCppVector<CppString> svec(machine,
				std::vector<std::string>{ "Hello,", "World!", "This string is long :)" });
			REQUIRE(svec->size() == 3);

			machine.vmcall("test", str, ivec, svec);

			// Check that the string was modified
			REQUIRE(str->to_string(machine) == "Hello, C++ World ..SSO..! Integers: 1 2 3 4 5 Strings: Hello, World! This string is long :)");
		}

		// Check that the number of active allocations is the same as before the test
		const unsigned allocs_now = machine.arena().allocation_counter() -
			machine.arena().deallocation_counter();
		REQUIRE(allocs_now == allocs_before);
	}

	SECTION("Test ScopedArenaObject with struct") {
		for (int i = 0; i < 10; i++) {
			// Scoped arena objects are guest-heap allocated, which means we can read back data
			// from the guest after the function call
			struct Data {
				int a, b, c, d;
			};
			ScopedArenaObject<Data> data(machine, Data{1, 2, 3, 4});

			machine.vmcall("test2", data);

			// Check that the struct was modified
			REQUIRE(data->a == 5);
			REQUIRE(data->b == 6);
			REQUIRE(data->c == 7);
			REQUIRE(data->d == 8);
		}

		const unsigned allocs_after2 = machine.arena().allocation_counter() -
			machine.arena().deallocation_counter();
		REQUIRE(allocs_after2 == allocs_before);
	}

	SECTION("Test nested GuestStdVector") {
		for (int i = 0; i < 10; i++) {
			ScopedCppVector<CppVector<int>> vec(machine);
			vec->push_back(machine, std::vector<int>{1, 2, 3});
			vec->push_back(machine, std::vector<int>{4, 5});
			REQUIRE(vec->size() == 2);
			REQUIRE(vec->capacity() >= 2);
			vec->clear(machine);
			REQUIRE(vec->empty());
			REQUIRE(vec->capacity() >= 2);
			vec->push_back(machine, std::vector<int>{1, 2, 3});
			vec->push_back(machine, std::vector<int>{4, 5});
			REQUIRE(vec->size() == 2);
			// Using reserve increases the capacity, but not the size
			vec->reserve(machine, 16);
			REQUIRE(vec->capacity() >= 16);
			REQUIRE(vec->size() == 2);
			// Check that the vectors were correctly initialized
			REQUIRE(vec->at(machine, 0).size() == 3);
			REQUIRE(vec->at(machine, 1).size() == 2);
			REQUIRE(vec->at(machine, 0).at(machine, 0) == 1);
			REQUIRE(vec->at(machine, 0).at(machine, 1) == 2);
			REQUIRE(vec->at(machine, 0).at(machine, 2) == 3);
			REQUIRE(vec->at(machine, 1).at(machine, 0) == 4);
			REQUIRE(vec->at(machine, 1).at(machine, 1) == 5);

			const int ret = machine.vmcall("test3", vec);

			// Check that the function returned the expected value
			REQUIRE(ret == 666);
			REQUIRE(vec->size() == 2);
			// We modified the second vector, adding an element
			REQUIRE(vec->at(machine, 1).size() == 3);
			REQUIRE(vec->at(machine, 1).at(machine, 2) == 666);

			// Test iterators (slightly more complex)
			size_t count = 0;
			auto begin = vec->begin(machine);
			auto end = vec->end(machine);
			for (auto it = begin; it != end; ++it) {
				auto& v = *it;
				for (size_t j = 0; j < v.size(); j++) {
					count += v.at(machine, j);
				}
			}
			REQUIRE(count == 1 + 2 + 3 + 4 + 5 + 666);
		}

		const unsigned allocs_after3 = machine.arena().allocation_counter() -
			machine.arena().deallocation_counter();
		REQUIRE(allocs_after3 == allocs_before);
	}
}

TEST_CASE("GuestStdString - SSO and heap allocation", "[native]")
{
	CompilerOptions opts;
	opts.optimization = 2;
	CodeBuilder builder;
	auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
		int main() { return 0; }
	)", "guest_string_test", opts);

	TestMachine test_machine(binary);
	Machine& machine = test_machine.machine();

	setup_native_system_calls(machine);
	test_machine.setup_linux();

	SECTION("Small String Optimization (SSO)") {
		ScopedCppString str(machine);
		REQUIRE(str->empty());

		// String fits in SSO buffer (15 bytes or less)
		str = "Small";
		REQUIRE(str->size == 5);
		REQUIRE(str->to_string(machine) == "Small");

		// Maximum SSO string
		str = "123456789012345"; // 15 characters
		REQUIRE(str->size == 15);
		REQUIRE(str->to_string(machine) == "123456789012345");
	}

	SECTION("Heap allocation for long strings") {
		ScopedCppString str(machine);

		// String exceeds SSO buffer
		const std::string long_str = "This is a very long string that exceeds SSO";
		str = long_str;
		REQUIRE(str->size == long_str.size());
		REQUIRE(str->to_string(machine) == long_str);
	}

	SECTION("String reassignment") {
		ScopedCppString str(machine);
		str = "First";
		REQUIRE(str->to_string(machine) == "First");

		str = "Second string longer";
		REQUIRE(str->to_string(machine) == "Second string longer");

		str = "Third";
		REQUIRE(str->to_string(machine) == "Third");
	}
}

TEST_CASE("GuestStdVector - basic operations", "[native]")
{
	CompilerOptions opts;
	opts.optimization = 2;
	CodeBuilder builder;
	auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
		int main() { return 0; }
	)", "guest_vector_test", opts);

	TestMachine test_machine(binary);
	Machine& machine = test_machine.machine();

	setup_native_system_calls(machine);
	test_machine.setup_linux();

	SECTION("Integer vector operations") {
		ScopedCppVector<int> vec(machine);
		REQUIRE(vec->empty());
		REQUIRE(vec->size() == 0);

		// Push back elements
		vec->push_back(machine, 1);
		vec->push_back(machine, 2);
		vec->push_back(machine, 3);
		REQUIRE(vec->size() == 3);
		REQUIRE(vec->at(machine, 0) == 1);
		REQUIRE(vec->at(machine, 1) == 2);
		REQUIRE(vec->at(machine, 2) == 3);

		// Pop back
		vec->pop_back(machine);
		REQUIRE(vec->size() == 2);
		REQUIRE(vec->at(machine, 0) == 1);
		REQUIRE(vec->at(machine, 1) == 2);

		// Clear
		vec->clear(machine);
		REQUIRE(vec->empty());
	}

	SECTION("Vector capacity and reserve") {
		ScopedCppVector<int> vec(machine);
		vec->reserve(machine, 100);
		REQUIRE(vec->capacity() >= 100);
		REQUIRE(vec->size() == 0);

		// Adding elements should not reallocate
		for (int i = 0; i < 50; i++) {
			vec->push_back(machine, i);
		}
		REQUIRE(vec->size() == 50);
		REQUIRE(vec->capacity() >= 100);
	}

	SECTION("Vector from std::vector") {
		std::vector<int> src = {10, 20, 30, 40, 50};
		ScopedCppVector<int> vec(machine, src);

		REQUIRE(vec->size() == 5);
		for (size_t i = 0; i < src.size(); i++) {
			REQUIRE(vec->at(machine, i) == src[i]);
		}
	}

	SECTION("String vector operations") {
		ScopedCppVector<CppString> svec(machine);
		svec->push_back(machine, "First");
		svec->push_back(machine, "Second string");
		svec->push_back(machine, "Third");

		REQUIRE(svec->size() == 3);
		REQUIRE(svec->at(machine, 0).to_string(machine) == "First");
		REQUIRE(svec->at(machine, 1).to_string(machine) == "Second string");
		REQUIRE(svec->at(machine, 2).to_string(machine) == "Third");

		// Convert to string vector
		auto result = svec->to_string_vector(machine);
		REQUIRE(result.size() == 3);
		REQUIRE(result[0] == "First");
		REQUIRE(result[1] == "Second string");
		REQUIRE(result[2] == "Third");
	}
}

TEST_CASE("GuestStdVector - resize operations", "[native]")
{
	CompilerOptions opts;
	opts.optimization = 2;
	CodeBuilder builder;
	auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
		int main() { return 0; }
	)", "guest_vector_resize", opts);

	TestMachine test_machine(binary);
	Machine& machine = test_machine.machine();

	setup_native_system_calls(machine);
	test_machine.setup_linux();

	ScopedCppVector<int> vec(machine, std::vector<int>{1, 2, 3, 4, 5});
	REQUIRE(vec->size() == 5);

	SECTION("Resize to larger size") {
		vec->resize(machine, 10);
		REQUIRE(vec->size() == 10);
		// Original elements preserved
		REQUIRE(vec->at(machine, 0) == 1);
		REQUIRE(vec->at(machine, 4) == 5);
		// New elements default-initialized to 0
		REQUIRE(vec->at(machine, 5) == 0);
		REQUIRE(vec->at(machine, 9) == 0);
	}

	SECTION("Resize to smaller size") {
		vec->resize(machine, 3);
		REQUIRE(vec->size() == 3);
		REQUIRE(vec->at(machine, 0) == 1);
		REQUIRE(vec->at(machine, 2) == 3);
	}
}
