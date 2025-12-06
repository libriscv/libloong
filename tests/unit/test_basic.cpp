#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
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

	SECTION("fcvt.x.x - conversion from float to gpr") {
		auto binary = builder.build(R"(
			int main() {}

			float convert_i32_float(int value) {
				return (float)value;
			}
			int convert_float_i32(float value) {
				return (int)value;
			}
			double convert_i32_double(int value) {
				return (double)value;
			}
			int convert_double_i32(double value) {
				return (int)value;
			}
			float convert_i64_float(long long value) {
				return (float)value;
			}
			long long convert_float_i64(float value) {
				return (long long)value;
			}
			double convert_i64_double(long long value) {
				return (double)value;
			}
			long long convert_double_i64(double value) {
				return (long long)value;
			}
		)", "fcvt_x_x_test");

		auto result = run_binary(binary, 0);
		REQUIRE(result.success);

		TestMachine machine(binary);
		machine.setup_linux();

		// Test int32 <-> float
		float fval = machine.vmcall<float>("convert_i32_float", 42);
		REQUIRE_THAT(fval, Catch::Matchers::WithinAbs(42.0f, 0.0001f));
		fval = machine.vmcall<float>("convert_i32_float", -42);
		REQUIRE_THAT(fval, Catch::Matchers::WithinAbs(-42.0f, 0.0001f));
		int ival = machine.vmcall<int>("convert_float_i32", 42.0f);
		REQUIRE(ival == 42);
		ival = machine.vmcall<int>("convert_float_i32", -42.0f);
		REQUIRE(ival == -42);
		// Test int32 <-> double
		double dval = machine.vmcall<double>("convert_i32_double", 42);
		REQUIRE_THAT(dval, Catch::Matchers::WithinAbs(42.0, 0.0001));
		dval = machine.vmcall<double>("convert_i32_double", -42);
		REQUIRE_THAT(dval, Catch::Matchers::WithinAbs(-42.0, 0.0001));
		ival = machine.vmcall<int>("convert_double_i32", 42.0);
		REQUIRE(ival == 42);
		ival = machine.vmcall<int>("convert_double_i32", -42.0);
		REQUIRE(ival == -42);
		// Test int64 <-> float
		fval = machine.vmcall<float>("convert_i64_float", 4200000000LL);
		REQUIRE_THAT(fval, Catch::Matchers::WithinAbs(4200000000.0f, 1e5f));
		fval = machine.vmcall<float>("convert_i64_float", -4200000000LL);
		REQUIRE_THAT(fval, Catch::Matchers::WithinAbs(-4200000000.0f, 1e5f));
		long long lval = machine.vmcall<long long>("convert_float_i64", 4200000000.0f);
		REQUIRE(lval == 4200000000LL);
		lval = machine.vmcall<long long>("convert_float_i64", -4200000000.0f);
		REQUIRE(lval == -4200000000LL);
		// Test int64 <-> double
		dval = machine.vmcall<double>("convert_i64_double", 4200000000LL);
		REQUIRE_THAT(dval, Catch::Matchers::WithinAbs(4200000000.0, 1e5));
		dval = machine.vmcall<double>("convert_i64_double", -4200000000LL);
		REQUIRE_THAT(dval, Catch::Matchers::WithinAbs(-4200000000.0, 1e5));
		lval = machine.vmcall<long long>("convert_double_i64", 4200000000.0);
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

	SECTION("arithmetic") {
		auto binary = builder.build(R"(
			int main() {}

			static float val = 0.0f;
			float get_value() {
				return val;
			}
			void set_value(float v) {
				val = v;
			}
			void fadd_value() {
				val += 1.0f;
			}
			void fsub_value() {
				val -= 1.0f;
			}
			void fmadd_value(float a, float b, float c) {
				val = a * b + c;
			}
			void fmadd_dv(double a, double b, double c) {
				val = (float)a * (float)b + (float)c;
			}
			int compare_lequal(float v) {
				return (val <= v) ? 1 : 0;
			}
			int compare_lequal_i(unsigned v) {
				return (val <= (float)v) ? 1 : 0;
			}

		)", "float_arithmetic_test");

		auto result = run_binary(binary, 0);
		REQUIRE(result.success);

		TestMachine machine(binary);
		machine.setup_linux();

		machine.vmcall("set_value", 10.0f);
		float val = machine.vmcall<float>("get_value");
		REQUIRE_THAT(val, Catch::Matchers::WithinAbs(10.0f, 1e-5f));
		machine.vmcall("fmadd_value", 2.0f, 3.0f, 4.0f);  // val = 2*3 +4 =10
		val = machine.vmcall<float>("get_value");
		REQUIRE_THAT(val, Catch::Matchers::WithinAbs(10.0f, 1e-5f));

		machine.vmcall("fmadd_dv", 1.0, 20.0, 22.0);  // val = 1*20 +22 =42
		val = machine.vmcall<float>("get_value");
		REQUIRE_THAT(val, Catch::Matchers::WithinAbs(42.0f, 1e-5f));

		machine.vmcall("set_value", 1.0f);
		machine.vmcall("fadd_value");  // val = 1 +1 =2
		val = machine.vmcall<float>("get_value");
		REQUIRE_THAT(val, Catch::Matchers::WithinAbs(2.0f, 1e-5f));
		machine.vmcall("fsub_value");  // val = 2 -1 =1
		val = machine.vmcall<float>("get_value");
		REQUIRE_THAT(val, Catch::Matchers::WithinAbs(1.0f, 1e-5f));

		machine.vmcall("set_value", 10.0f);
		int cmp = machine.vmcall<int>("compare_lequal", 10.0f);
		REQUIRE(cmp == 1);
		cmp = machine.vmcall<int>("compare_lequal", 9.0f);
		REQUIRE(cmp == 0);
		cmp = machine.vmcall<int>("compare_lequal_i", 10u);
		REQUIRE(cmp == 1);
		cmp = machine.vmcall<int>("compare_lequal_i", 9u);
		REQUIRE(cmp == 0);
	}
}
