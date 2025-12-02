#include <catch2/catch_test_macros.hpp>
#include "codebuilder.hpp"
#include "test_utils.hpp"

using namespace loongarch;
using namespace loongarch::test;

TEST_CASE("C++ basic features", "[cpp]") {
	CodeBuilder builder;

	SECTION("Simple C++ program") {
		auto binary = builder.build_cpp(R"(
			int main() {
				return 42;
			}
		)", "cpp_simple");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}

	SECTION("C++ classes") {
		auto binary = builder.build_cpp(R"(
			class Calculator {
			public:
				int add(int a, int b) {
					return a + b;
				}

				int multiply(int a, int b) {
					return a * b;
				}
			};

			int main() {
				Calculator calc;
				int result = calc.add(10, 32);
				return result;
			}
		)", "cpp_class");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}

	SECTION("C++ constructors") {
		auto binary = builder.build_cpp(R"(
			class Counter {
			private:
				int value;
			public:
				Counter(int v) : value(v) {}
				int get() { return value; }
				void increment() { value++; }
			};

			int main() {
				Counter c(40);
				c.increment();
				c.increment();
				return c.get();
			}
		)", "cpp_constructor");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}
}

TEST_CASE("C++ inheritance", "[cpp][inheritance]") {
	CodeBuilder builder;

	SECTION("Simple inheritance") {
		auto binary = builder.build_cpp(R"(
			class Base {
			public:
				int getValue() { return 20; }
			};

			class Derived : public Base {
			public:
				int getDouble() { return getValue() * 2; }
			};

			int main() {
				Derived d;
				return d.getDouble() + 2;  // 40 + 2 = 42
			}
		)", "cpp_inheritance");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}

	SECTION("Virtual functions") {
		auto binary = builder.build_cpp(R"(
			class Shape {
			public:
				virtual int getValue() { return 10; }
			};

			class Circle : public Shape {
			public:
				int getValue() override { return 42; }
			};

			int main() {
				Circle c;
				Shape* s = &c;
				return s->getValue();
			}
		)", "cpp_virtual");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}
}

TEST_CASE("C++ templates", "[cpp][templates]") {
	CodeBuilder builder;

	SECTION("Function template") {
		auto binary = builder.build_cpp(R"(
			template<typename T>
			T add(T a, T b) {
				return a + b;
			}

			int main() {
				int result = add(15, 27);
				return result;
			}
		)", "cpp_template_func");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}

	SECTION("Class template") {
		auto binary = builder.build_cpp(R"(
			template<typename T>
			class Container {
			private:
				T value;
			public:
				Container(T v) : value(v) {}
				T get() { return value; }
			};

			int main() {
				Container<int> c(42);
				return c.get();
			}
		)", "cpp_template_class");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}
}

TEST_CASE("C++ standard library", "[cpp][stdlib]") {
	CodeBuilder builder;

	// TODO: iostream with std::cout has issues - investigate
	/*
	SECTION("iostream") {
		auto binary = builder.build_cpp(R"(
			#include <iostream>

			int main() {
				std::cout << "Hello from C++!" << std::endl;
				return 42;
			}
		)", "cpp_iostream");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}
	*/

	SECTION("string manipulation") {
		auto binary = builder.build_cpp(R"(
			#include <string>

			int main() {
				std::string s1 = "Hello";
				std::string s2 = "World";
				std::string result = s1 + " " + s2;
				return result.length();  // "Hello World" = 11 chars
			}
		)", "cpp_string");

		auto result = run_binary(binary, 11);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 11);
	}
}

TEST_CASE("C++ operator overloading", "[cpp][operators]") {
	CodeBuilder builder;

	SECTION("Operator overloading") {
		auto binary = builder.build_cpp(R"(
			class Number {
			private:
				int val;
			public:
				Number(int v) : val(v) {}
				Number operator+(const Number& other) {
					return Number(val + other.val);
				}
				int get() { return val; }
			};

			int main() {
				Number a(15);
				Number b(27);
				Number c = a + b;
				return c.get();
			}
		)", "cpp_operator");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}
}

TEST_CASE("C++ exceptions", "[cpp][exceptions]") {
	CodeBuilder builder;

	SECTION("Try-catch") {
		auto binary = builder.build_cpp(R"(
			int divide(int a, int b) {
				if (b == 0) throw 99;
				return a / b;
			}

			int main() {
				try {
					return divide(84, 2);
				} catch (int e) {
					return e;
				}
			}
		)", "cpp_exception");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}

	SECTION("Exception thrown") {
		auto binary = builder.build_cpp(R"(
			int divide(int a, int b) {
				if (b == 0) throw 42;
				return a / b;
			}

			int main() {
				try {
					return divide(10, 0);
				} catch (int e) {
					return e;
				}
			}
		)", "cpp_exception_thrown");

		auto result = run_binary(binary, 42);
		REQUIRE(result.success);
		REQUIRE(result.exit_code == 42);
	}
}
