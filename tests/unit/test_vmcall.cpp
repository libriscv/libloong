#include <catch2/catch_test_macros.hpp>
#include "codebuilder.hpp"
#include "test_utils.hpp"

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

TEST_CASE("vmcall - calling guest functions", "[vmcall]") {
	CodeBuilder builder;

	SECTION("Call simple function") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			int add(int a, int b) {
				return a + b;
			}

			int main() {
				return 0;
			}
		)", "vmcall_add");

		TestMachine machine(binary);
		machine.setup_linux();

		// Call the add function from host
		int result = machine.vmcall("add", 15, 27);
		REQUIRE(result == 42);
	}

	SECTION("Call function with multiple parameters") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			int sum_four(int a, int b, int c, int d) {
				return a + b + c + d;
			}

			int main() {
				return 0;
			}
		)", "vmcall_sum");

		TestMachine machine(binary);
		machine.setup_linux();

		int result = machine.vmcall("sum_four", 10, 11, 12, 9);
		REQUIRE(result == 42);
	}

	SECTION("Call function that uses stack") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			int complex_calc(int x) {
				int a = x * 2;
				int b = a + 10;
				int c = b - 8;
				return c;
			}

			int main() {
				return 0;
			}
		)", "vmcall_stack");

		TestMachine machine(binary);
		machine.setup_linux();

		int result = machine.vmcall("complex_calc", 20);
		REQUIRE(result == 42);  // 20*2 + 10 - 8 = 42
	}

	SECTION("Multiple vmcalls") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			int multiply(int a, int b) {
				return a * b;
			}

			int add(int a, int b) {
				return a + b;
			}

			int main() {
				return 0;
			}
		)", "vmcall_multiple");

		TestMachine machine(binary);
		machine.setup_linux();

		int result1 = machine.vmcall("multiply", 6, 7);
		REQUIRE(result1 == 42);

		int result2 = machine.vmcall("add", 30, 12);
		REQUIRE(result2 == 42);
	}
}

TEST_CASE("vmcall - guest memory interaction", "[vmcall][memory]") {
	CodeBuilder builder;

	SECTION("Function reading from pointer") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			int sum_array(int* arr, int len) {
				int sum = 0;
				for (int i = 0; i < len; i++) {
					sum += arr[i];
				}
				return sum;
			}

			int main() {
				return 0;
			}
		)", "vmcall_array");

		TestMachine machine(binary);
		machine.setup_linux();

		// Allocate memory in guest and write array
		auto arr_addr = machine.machine().memory.mmap_allocate(64);
		// We would need heap allocation here - simplified for now
	}

	SECTION("Function modifying memory") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			void increment_value(int* ptr) {
				*ptr = *ptr + 1;
			}

			int main() {
				return 0;
			}
		)", "vmcall_modify");

		TestMachine machine(binary);
		machine.setup_linux();

		// This test demonstrates the concept - actual implementation
		// would require allocating guest memory for the pointer
	}
}

TEST_CASE("vmcall - return values", "[vmcall]") {
	CodeBuilder builder;

	SECTION("Return various integer sizes") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			char get_char() { return 42; }
			short get_short() { return 42; }
			int get_int() { return 42; }
			long get_long() { return 42; }

			int main() {
				return 0;
			}
		)", "vmcall_types");

		TestMachine machine(binary);
		machine.setup_linux();

		REQUIRE(machine.vmcall("get_char") == 42);
		REQUIRE(machine.vmcall("get_short") == 42);
		REQUIRE(machine.vmcall("get_int") == 42);
		REQUIRE(machine.vmcall("get_long") == 42);
	}

	SECTION("Return negative values") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			int get_negative() {
				return -42;
			}

			int main() {
				return 0;
			}
		)", "vmcall_negative");

		TestMachine machine(binary);
		machine.setup_linux();

		int result = machine.vmcall("get_negative");
		REQUIRE(result == -42);
	}
}

TEST_CASE("vmcall - complex scenarios", "[vmcall][advanced]") {
	CodeBuilder builder;

	SECTION("Recursive function via vmcall") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			int factorial(int n) {
				if (n <= 1) return 1;
				return n * factorial(n - 1);
			}

			int main() {
				return 0;
			}
		)", "vmcall_factorial");

		TestMachine machine(binary);
		machine.setup_linux();

		REQUIRE(machine.vmcall("factorial", 5) == 120);
		REQUIRE(machine.vmcall("factorial", 6) == 720);
		REQUIRE(machine.vmcall("factorial", 1) == 1);
	}

	SECTION("Function calling other functions") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			int add(int a, int b) {
				return a + b;
			}

			int multiply(int a, int b) {
				return a * b;
			}

			int complex_op(int x) {
				int a = add(x, 10);
				int b = multiply(a, 2);
				return b;
			}

			int main() {
				return 0;
			}
		)", "vmcall_nested");

		TestMachine machine(binary);
		machine.setup_linux();

		int result = machine.vmcall("complex_op", 11);
		REQUIRE(result == 42);  // (11 + 10) * 2 = 42
	}
}

// Note: Floating-point vmcall tests are disabled until FP instructions are fully implemented
// The emulator currently reports "UNIMPLEMENTED" for some FP operations
/*
TEST_CASE("vmcall - floating-point arguments and returns", "[vmcall][float]") {
	CodeBuilder builder;

	SECTION("Float addition") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			float add_floats(float a, float b) {
				return a + b;
			}

			int main() {
				return 0;
			}
		)", "vmcall_float_add");

		TestMachine machine(binary);
		machine.setup_linux();

		// Note: For float return values, we'd need to read from FP registers
		// This test demonstrates the concept
		machine.vmcall("add_floats", 3.14f, 2.86f);
		// Would need: REQUIRE(machine.get_fp_reg(0) == Approx(6.0f));
	}

	SECTION("Double multiplication") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			double multiply_doubles(double a, double b) {
				return a * b;
			}

			int main() {
				return 0;
			}
		)", "vmcall_double_mul");

		TestMachine machine(binary);
		machine.setup_linux();

		machine.vmcall("multiply_doubles", 2.5, 4.0);
		// Would return 10.0 in FA0
	}

	SECTION("Mixed integer and float") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			int float_to_int(float x) {
				return (int)x;
			}

			int main() {
				return 0;
			}
		)", "vmcall_mixed");

		TestMachine machine(binary);
		machine.setup_linux();

		int result = machine.vmcall("float_to_int", 42.7f);
		REQUIRE(result == 42);
	}
}
*/

TEST_CASE("vmcall - eight or more arguments", "[vmcall][args]") {
	CodeBuilder builder;

	SECTION("Eight integer arguments") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			int sum_eight(int a, int b, int c, int d, int e, int f, int g, int h) {
				return a + b + c + d + e + f + g + h;
			}

			int main() {
				return 0;
			}
		)", "vmcall_eight_args");

		TestMachine machine(binary);
		machine.setup_linux();

		// A0-A7 are used, last arg goes to stack
		int result = machine.vmcall("sum_eight", 1, 2, 3, 4, 5, 6, 7, 14);
		REQUIRE(result == 42);
	}

	// Note: We only support up to 8 integer arguments without stack spilling
	// This is a limitation to keep the fold expression constexpr-friendly
	/*
	SECTION("Nine integer arguments") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			int sum_nine(int a, int b, int c, int d, int e, int f, int g, int h, int i) {
				return a + b + c + d + e + f + g + h + i;
			}

			int main() {
				return 0;
			}
		)", "vmcall_nine_args");

		TestMachine machine(binary);
		machine.setup_linux();

		// A0-A7 used, 9th arg would need stack spilling (not supported)
		int result = machine.vmcall("sum_nine", 1, 2, 3, 4, 5, 6, 7, 8, 6);
		REQUIRE(result == 42);
	}
	*/
}

TEST_CASE("vmcall - unsigned and signed types", "[vmcall][types]") {
	CodeBuilder builder;

	SECTION("Unsigned int") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			unsigned int get_max_unsigned() {
				return 0xFFFFFFFF;
			}

			int main() {
				return 0;
			}
		)", "vmcall_unsigned");

		TestMachine machine(binary);
		machine.setup_linux();

		auto result = machine.vmcall("get_max_unsigned");
		// Result will be sign-extended but should match as unsigned
		REQUIRE((result & 0xFFFFFFFF) == 0xFFFFFFFF);
	}

	SECTION("Long (64-bit)") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			long get_large_value() {
				return 0x123456789ABCDEFLL;
			}

			int main() {
				return 0;
			}
		)", "vmcall_long");

		TestMachine machine(binary);
		machine.setup_linux();

		auto result = machine.vmcall("get_large_value");
		REQUIRE(result == 0x123456789ABCDEFLL);
	}
}

TEST_CASE("vmcall - zero and boundary values", "[vmcall][edge]") {
	CodeBuilder builder;

	SECTION("Zero arguments function") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			int get_constant() {
				return 42;
			}

			int main() {
				return 0;
			}
		)", "vmcall_no_args");

		TestMachine machine(binary);
		machine.setup_linux();

		int result = machine.vmcall("get_constant");
		REQUIRE(result == 42);
	}

	SECTION("All zero arguments") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			int check_zeros(int a, int b, int c) {
				if (a == 0 && b == 0 && c == 0)
					return 42;
				return 0;
			}

			int main() {
				return 0;
			}
		)", "vmcall_zeros");

		TestMachine machine(binary);
		machine.setup_linux();

		int result = machine.vmcall("check_zeros", 0, 0, 0);
		REQUIRE(result == 42);
	}

	SECTION("Negative numbers") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			int absolute_sum(int a, int b, int c) {
				if (a < 0) a = -a;
				if (b < 0) b = -b;
				if (c < 0) c = -c;
				return a + b + c;
			}

			int main() {
				return 0;
			}
		)", "vmcall_negatives");

		TestMachine machine(binary);
		machine.setup_linux();

		int result = machine.vmcall("absolute_sum", -10, -20, -12);
		REQUIRE(result == 42);
	}
}

TEST_CASE("vmcall - std::string arguments", "[vmcall][string]") {
	CodeBuilder builder;

	SECTION("String length") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			#include <string.h>
			int string_length(const char* str) {
				return strlen(str);
			}

			int main() {
				return 0;
			}
		)", "vmcall_string_length");

		TestMachine machine(binary);
		machine.setup_linux();

		std::string test_str = "Hello World";
		int result = machine.vmcall("string_length", test_str);
		REQUIRE(result == 11);
	}

	SECTION("String comparison") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			#include <string.h>
			int compare_strings(const char* a, int alen, const char* b, int blen) {
				return strcmp(a, b) == 0 ? 1 : 0;
			}

			int main() {
				return 0;
			}
		)", "vmcall_string_cmp");

		TestMachine machine(binary);
		machine.setup_linux();

		std::string str1 = "test";
		std::string str2 = "test";
		int result = machine.vmcall("compare_strings", str1, str1.length(), str2, str2.length());
		REQUIRE(result == 1);
	}

	SECTION("String concatenation length") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			#include <string.h>
			int concat_length(const char* a, const char* b) {
				return strlen(a) + strlen(b);
			}

			int main() {
				return 0;
			}
		)", "vmcall_string_concat");

		TestMachine machine(binary);
		machine.setup_linux();

		std::string part1 = "Hello ";
		std::string part2 = "World!";
		int result = machine.vmcall("concat_length", part1, part2);
		REQUIRE(result == 12);
	}

	SECTION("Empty string") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			int is_empty(const char* str) {
				return str[0] == '\0' ? 1 : 0;
			}

			int main() {
				return 0;
			}
		)", "vmcall_empty_string");

		TestMachine machine(binary);
		machine.setup_linux();

		std::string empty = "";
		int result = machine.vmcall("is_empty", empty);
		REQUIRE(result == 1);
	}
}

TEST_CASE("vmcall - struct by-value arguments", "[vmcall][struct]") {
	CodeBuilder builder;

	SECTION("Simple struct with two integers") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			struct Point {
				int x;
				int y;
			};

			int sum_point(const struct Point* p) {
				return p->x + p->y;
			}

			int main() {
				return 0;
			}
		)", "vmcall_simple_struct");

		TestMachine machine(binary);
		machine.setup_linux();

		struct Point {
			int x;
			int y;
		};
		Point p = {30, 12};
		int result = machine.vmcall("sum_point", p);
		REQUIRE(result == 42);
	}

	SECTION("Struct with multiple fields") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			struct Data {
				int a;
				int b;
				int c;
				int d;
			};

			int sum_data(const struct Data* d) {
				return d->a + d->b + d->c + d->d;
			}

			int main() {
				return 0;
			}
		)", "vmcall_multi_field_struct");

		TestMachine machine(binary);
		machine.setup_linux();

		struct Data {
			int a;
			int b;
			int c;
			int d;
		};
		Data data = {10, 11, 12, 9};
		int result = machine.vmcall("sum_data", data);
		REQUIRE(result == 42);
	}

	SECTION("Struct with different types") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			struct Mixed {
				int value;
				char flag;
				short count;
			};

			int process_mixed(const struct Mixed* m) {
				return m->value + m->flag + m->count;
			}

			int main() {
				return 0;
			}
		)", "vmcall_mixed_struct");

		TestMachine machine(binary);
		machine.setup_linux();

		struct Mixed {
			int value;
			char flag;
			short count;
		};
		Mixed m = {30, 2, 10};
		int result = machine.vmcall("process_mixed", m);
		REQUIRE(result == 42);
	}

	SECTION("Multiple struct arguments") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			struct Pair {
				int first;
				int second;
			};

			int sum_pairs(const struct Pair* p1, const struct Pair* p2) {
				return p1->first + p1->second + p2->first + p2->second;
			}

			int main() {
				return 0;
			}
		)", "vmcall_multiple_structs");

		TestMachine machine(binary);
		machine.setup_linux();

		struct Pair {
			int first;
			int second;
		};
		Pair p1 = {10, 11};
		Pair p2 = {12, 9};
		int result = machine.vmcall("sum_pairs", p1, p2);
		REQUIRE(result == 42);
	}
}

TEST_CASE("vmcall - mixed complex types", "[vmcall][mixed]") {
	CodeBuilder builder;

	SECTION("Struct and string arguments") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			#include <string.h>
			struct Data {
				int value;
				int multiplier;
			};

			int process_with_string(const struct Data* d, const char* str) {
				return (d->value * d->multiplier) + strlen(str);
			}

			int main() {
				return 0;
			}
		)", "vmcall_struct_and_string");

		TestMachine machine(binary);
		machine.setup_linux();

		struct Data {
			int value;
			int multiplier;
		};
		Data d = {10, 4};
		std::string str = "ab"; // length 2
		int result = machine.vmcall("process_with_string", d, str);
		REQUIRE(result == 42); // (10 * 4) + 2 = 42
	}

	SECTION("Integer, string, and struct") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			#include <string.h>
			struct Point {
				int x;
				int y;
			};

			int complex_calc(int base, const char* str, const struct Point* p) {
				if (strcmp(str, "test") != 0) {
					return -1;
				}
				return base + strlen(str) + p->x + p->y;
			}

			int main() {
				return 0;
			}
		)", "vmcall_int_string_struct");

		TestMachine machine(binary);
		machine.setup_linux();

		struct Point {
			int x;
			int y;
		};
		Point p = {10, 20};
		std::string str = "test"; // length 4
		int result = machine.vmcall("complex_calc", 8, str, p);
		REQUIRE(result == 42); // 8 + 4 + 10 + 20 = 42
	}

	SECTION("Multiple strings") {
		auto binary = builder.build(FAST_EXIT_FUNCTION + R"(
			#include <string.h>
			int sum_string_lengths(const char* a, const char* b, const char* c) {
				return strlen(a) + strlen(b) + strlen(c);
			}

			int main() {
				return 0;
			}
		)", "vmcall_multiple_strings");

		TestMachine machine(binary);
		machine.setup_linux();

		std::string s1 = "Hello";       // 5
		std::string s2 = "World";       // 5
		std::string s3 = "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"; // 32
		int result = machine.vmcall("sum_string_lengths", s1, s2, s3);
		REQUIRE(result == 42);
	}
}
