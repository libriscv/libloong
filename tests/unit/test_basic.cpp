#include <catch2/catch_test_macros.hpp>
#include "codebuilder.hpp"
#include "test_utils.hpp"

using namespace loongarch;
using namespace loongarch::test;

TEST_CASE("Basic C program execution", "[basic]") {
	CodeBuilder builder;

	SECTION("Simple return value") {
		auto binary = builder.build(R"(
			int main() {
				return 42;
			}
		)", "simple_return");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
		REQUIRE(result.instructions_executed > 0);
	}

	SECTION("Arithmetic operations") {
		auto binary = builder.build(R"(
			int main() {
				int a = 15;
				int b = 27;
				return a + b;
			}
		)", "arithmetic");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}

	SECTION("Local variables") {
		auto binary = builder.build(R"(
			int main() {
				int x = 10;
				int y = 20;
				int z = 30;
				return x + y + z - 18;
			}
		)", "local_vars");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}

	SECTION("RDTIME.D") {
		auto binary = builder.build(R"(
			int main() {
				unsigned long t1;
				asm volatile("rdtime.d %0, $zero" : "=r"(t1));
				return (t1 != 0) ? 42 : 1;
			}
		)", "rdtime_test");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}
}

TEST_CASE("Control flow", "[basic][control]") {
	CodeBuilder builder;

	SECTION("If statement") {
		auto binary = builder.build(R"(
			int main() {
				int x = 10;
				if (x == 10) {
					return 42;
				}
				return 1;
			}
		)", "if_statement");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}

	SECTION("If-else statement") {
		auto binary = builder.build(R"(
			int main() {
				int x = 5;
				if (x > 10) {
					return 1;
				} else {
					return 42;
				}
			}
		)", "if_else");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}

	SECTION("For loop") {
		auto binary = builder.build(R"(
			int main() {
				int sum = 0;
				for (int i = 0; i < 10; i++) {
					sum += i;
				}
				return sum - 3;  // 0+1+2+...+9 = 45, return 42
			}
		)", "for_loop");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}

	SECTION("While loop") {
		auto binary = builder.build(R"(
			int main() {
				int i = 0;
				int sum = 0;
				while (i < 10) {
					sum += i;
					i++;
				}
				return sum - 3;
			}
		)", "while_loop");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}
}

TEST_CASE("Functions", "[basic][functions]") {
	CodeBuilder builder;

	SECTION("Simple function call") {
		auto binary = builder.build(R"(
			int add(int a, int b) {
				return a + b;
			}

			int main() {
				return add(15, 27);
			}
		)", "function_call");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}

	SECTION("Recursive function - factorial") {
		auto binary = builder.build(R"(
			int factorial(int n) {
				if (n <= 1) return 1;
				return n * factorial(n - 1);
			}

			int main() {
				int result = factorial(5);  // 120
				return result / 10 + 30;     // 12 + 30 = 42
			}
		)", "factorial");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}

	SECTION("Fibonacci") {
		auto binary = builder.build(R"(
			int fib(int n) {
				if (n <= 1) return n;
				return fib(n - 1) + fib(n - 2);
			}

			int main() {
				return fib(9) + 8;  // fib(9) = 34, +8 = 42
			}
		)", "fibonacci");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}
}

TEST_CASE("Arrays and pointers", "[basic][memory]") {
	CodeBuilder builder;

	SECTION("Array access") {
		auto binary = builder.build(R"(
			int main() {
				int arr[5] = {10, 20, 30, 40, 50};
				return arr[1] + arr[3] - 18;  // 20 + 40 - 18 = 42
			}
		)", "array_access");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}

	SECTION("Array modification") {
		auto binary = builder.build(R"(
			int main() {
				int arr[3] = {1, 2, 3};
				arr[0] = 10;
				arr[1] = 20;
				arr[2] = 12;
				return arr[0] + arr[1] + arr[2];  // 42
			}
		)", "array_modify");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}

	SECTION("Pointer arithmetic") {
		auto binary = builder.build(R"(
			int main() {
				int arr[3] = {10, 20, 12};
				int *p = arr;
				int sum = *p + *(p+1) + *(p+2);
				return sum;
			}
		)", "pointer_arithmetic");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}
}

TEST_CASE("stdio functions", "[basic][stdio]") {
	CodeBuilder builder;

	SECTION("printf") {
		auto binary = builder.build(R"(
			#include <stdio.h>
			int main() {
				printf("Hello from LoongArch!\n");
				return 0;
			}
		)", "printf_test");

		auto result = run_binary(binary, 0);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 0);
	}

	SECTION("Multiple printf calls") {
		auto binary = builder.build(R"(
			#include <stdio.h>
			int main() {
				printf("Line 1\n");
				printf("Line 2\n");
				printf("Line 3\n");
				return 42;
			}
		)", "multi_printf");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}
}

TEST_CASE("Floating-point operations", "[basic][float]") {
	CodeBuilder builder;

	SECTION("fcmp.ceq.d - double precision equality comparison") {
		auto binary = builder.build(R"(
			int main() {
				double a = 3.14159;
				double b = 3.14159;
				if (a == b) {
					return 42;
				}
				return 0;
			}
		)", "fcmp_ceq_test");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}

	SECTION("LASX vectorized initialization") {
		CompilerOptions opts;
		opts.optimization = 2;
		opts.extra_flags = {"-mlasx"};

		auto binary = builder.build(R"(
			#include <float.h>
			#include <stdio.h>
			int main() {
				double arr[128] __attribute__((aligned(32)));

				// Initialize array - compiler should vectorize with LASX
				for (int i = 0; i < 128; i++) {
					arr[i] = 1.0;
				}
				double min_val = FLT_MAX;
				double max_val = FLT_MIN;

				// Verify all elements
				for (int i = 0; i < 128; i++) {
					if (arr[i] != 1.0) {
						return 1;
					}
					min_val = (arr[i] < min_val) ? arr[i] : min_val;
					max_val = (arr[i] > max_val) ? arr[i] : max_val;
				}
				if (min_val != 1.0 || max_val != 1.0) {
					return 1;
				}
				return 0;
			}
		)", "lasx_vector_init", opts);

		auto result = run_binary(binary, 0);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 0);
	}

	SECTION("LASX vector add, mul and fmadd") {
		CompilerOptions opts;
		opts.optimization = 2;
		opts.extra_flags = {"-mlasx"};

		auto binary = builder.build(R"(
			int main() {
				volatile double arr[128] __attribute__((aligned(32)));
				volatile double arr2[128] __attribute__((aligned(32)));
				volatile double result[128] __attribute__((aligned(32)));

				// Initialize array - compiler should vectorize with LASX
				for (int i = 0; i < 128; i++) {
					arr[i] = 1.0;
					arr2[i] = 1.0;
				}

				// Vectorized addition: result[i] = arr[i] + arr[i]
				for (int i = 0; i < 128; i++) {
					result[i] = arr[i] + arr[i];
				}
				asm("" ::: "memory");  // Prevent optimization away
				for (int i = 0; i < 128; i++) {
					if (result[i] != 2.0)
						return 1;
				}
				// Vectorized multiplication: result[i] = arr[i] * 3.0
				for (int i = 0; i < 128; i++) {
					result[i] = arr[i] * 3.0;
				}
				asm("" ::: "memory");  // Prevent optimization away
				for (int i = 0; i < 128; i++) {
					if (result[i] != 3.0)
						return 1;
				}
				// Vectorized fused multiply-add: result[i] = arr[i] * 4.0 + 2.0
				for (int i = 0; i < 128; i++) {
					result[i] = arr[i] * 4.0 + arr2[i];
				}
				asm("" ::: "memory");  // Prevent optimization away
				for (int i = 0; i < 128; i++) {
					if (result[i] != 5.0)
						return 1;
				}
				return 0;
			}
		)", "lasx_vector_add_mul_fmadd", opts);

		auto result = run_binary(binary, 0);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 0);
	}
}
