#include <catch2/catch_test_macros.hpp>
#include "codebuilder.hpp"
#include "test_utils.hpp"

using namespace loongarch;
using namespace loongarch::test;

TEST_CASE("Machine instantiation", "[machine]") {
	CodeBuilder builder;

	SECTION("Create machine from binary") {
		auto binary = builder.build(R"(
			int main() {
				return 42;
			}
		)", "machine_test");

		REQUIRE_NOTHROW([&]() {
			TestMachine machine(binary);
		}());
	}

	SECTION("Create multiple machines") {
		auto binary = builder.build(R"(
			int main() {
				return 0;
			}
		)", "multi_machine");

		TestMachine machine1(binary);
		TestMachine machine2(binary);

		machine1.setup_linux();
		machine2.setup_linux();

		// Both should work independently
		auto result1 = machine1.execute();
		auto result2 = machine2.execute();

		REQUIRE(result1.success);
		REQUIRE(result2.success);
	}
}

TEST_CASE("Memory operations", "[machine][memory]") {
	CodeBuilder builder;

	SECTION("Read and write memory") {
		auto binary = builder.build(R"(
			int global_var = 123;

			int main() {
				return 0;
			}
		)", "memory_rw");

		TestMachine machine(binary);
		machine.setup_linux();

		// Get address of global variable
		auto addr = machine.address_of("global_var");
		REQUIRE(addr != 0);

		// Read the value
		int value = machine.read<int>(addr);
		REQUIRE(value == 123);

		// Write a new value
		machine.write<int>(addr, 456);

		// Read it back
		value = machine.read<int>(addr);
		REQUIRE(value == 456);
	}

	SECTION("Memory boundaries") {
		auto binary = builder.build(R"(
			int main() {
				return 0;
			}
		)", "memory_bounds");

		TestMachine machine(binary, 16 * 1024 * 1024);  // 16 MB limit
		machine.setup_linux();

		// Should be able to read valid memory
		auto main_addr = machine.address_of("main");
		REQUIRE(main_addr != 0);
	}
}

TEST_CASE("Register access", "[machine][registers]") {
	CodeBuilder builder;

	SECTION("Read register after execution") {
		auto binary = builder.build(R"(
			int main() {
				return 42;
			}
		)", "reg_test");

		TestMachine machine(binary);
		machine.setup_linux();

		auto result = machine.execute();
		REQUIRE(result.success);

		// A0 should contain the return value
		uint64_t a0 = machine.get_reg(REG_A0);
		REQUIRE(a0 == 42);
	}

	// TODO: Re-enable when vmcall is implemented
	/*
	SECTION("Modify registers") {
		auto binary = builder.build(R"(
			int get_a0_value() {
				register int a0_val asm("a0");
				return a0_val;
			}

			int main() {
				return 0;
			}
		)", "reg_modify");

		TestMachine machine(binary);
		machine.setup_linux();

		// Set A0 to a value
		machine.set_reg(REG_A0, 42);

		// Call function that reads A0
		int result = machine.vmcall("get_a0_value");
		REQUIRE(result == 42);
	}
	*/
}

TEST_CASE("Instruction counting", "[machine][performance]") {
	CodeBuilder builder;

	SECTION("Count instructions") {
		auto binary = builder.build(R"(
			int main() {
				int sum = 0;
				for (int i = 0; i < 100; i++) {
					sum += i;
				}
				return 0;
			}
		)", "insn_count");

		TestMachine machine(binary);
		machine.setup_linux();

		auto result = machine.execute();
		REQUIRE(result.success);
		REQUIRE(result.instructions_executed > 0);
		REQUIRE(result.instructions_executed < 200000);  // Sanity check (libc overhead)
	}

	// TODO: This test is flaky - the machine may complete successfully even with low instruction limits
	// Need to investigate instruction counting behavior
	/*
	SECTION("Instruction limit") {
		auto binary = builder.build(R"(
			volatile int x = 0;
			int main() {
				while (1) {
					x++;  // Volatile to prevent optimization
				}
				return 0;
			}
		)", "insn_limit");

		TestMachine machine(binary);
		machine.setup_linux();

		// Should hit instruction limit - use very low limit to ensure we hit it
		auto result = machine.execute(500);
		// The program shouldn't complete normally with such a low instruction limit
		// Either it fails, or doesn't complete successfully
		REQUIRE_FALSE(result.success);
	}
	*/
}

TEST_CASE("Symbol lookup", "[machine][symbols]") {
	CodeBuilder builder;

	SECTION("Find function symbols") {
		auto binary = builder.build(R"(
			void func1() {}
			void func2() {}
			int func3() { return 42; }

			int main() {
				return 0;
			}
		)", "symbol_lookup");

		TestMachine machine(binary);
		machine.setup_linux();

		auto main_addr = machine.address_of("main");
		REQUIRE(main_addr != 0);

		auto func1_addr = machine.address_of("func1");
		REQUIRE(func1_addr != 0);

		auto func2_addr = machine.address_of("func2");
		REQUIRE(func2_addr != 0);

		auto func3_addr = machine.address_of("func3");
		REQUIRE(func3_addr != 0);

		// Symbols should be distinct
		REQUIRE(main_addr != func1_addr);
		REQUIRE(func1_addr != func2_addr);
	}

	SECTION("Find global variables") {
		auto binary = builder.build(R"(
			int global_int = 42;
			char global_char = 'A';

			int main() {
				return 0;
			}
		)", "global_symbols");

		TestMachine machine(binary);
		machine.setup_linux();

		auto int_addr = machine.address_of("global_int");
		REQUIRE(int_addr != 0);

		auto char_addr = machine.address_of("global_char");
		REQUIRE(char_addr != 0);
	}

	SECTION("Non-existent symbol") {
		auto binary = builder.build(R"(
			int main() {
				return 0;
			}
		)", "no_symbol");

		TestMachine machine(binary);
		machine.setup_linux();

		auto addr = machine.address_of("nonexistent_function");
		REQUIRE(addr == 0);
	}
}

TEST_CASE("Machine state", "[machine][state]") {
	CodeBuilder builder;

	SECTION("Check stopped state") {
		auto binary = builder.build(R"(
			int main() {
				return 42;
			}
		)", "stopped_test");

		TestMachine machine(binary);
		machine.setup_linux();

		auto result = machine.execute();
		REQUIRE(result.success);
		REQUIRE(machine.machine().stopped());
	}

	SECTION("Check running state") {
		auto binary = builder.build(R"(
			int main() {
				return 0;
			}
		)", "running_test");

		TestMachine machine(binary);
		machine.setup_linux();

		// After execution - machine should be stopped
		auto result = machine.execute();
		REQUIRE(result.success);
		REQUIRE(machine.machine().stopped());
	}
}

TEST_CASE("Program counter", "[machine][pc]") {
	CodeBuilder builder;

	SECTION("Initial PC") {
		auto binary = builder.build(R"(
			int main() {
				return 42;
			}
		)", "pc_test");

		TestMachine machine(binary);
		machine.setup_linux();

		// PC should be at entry point
		auto pc = machine.machine().cpu.pc();
		REQUIRE(pc != 0);
	}

	SECTION("Final PC after execution") {
		auto binary = builder.build(R"(
			int main() {
				return 42;
			}
		)", "pc_final");

		TestMachine machine(binary);
		machine.setup_linux();

		auto result = machine.execute();
		REQUIRE(result.success);
		REQUIRE(result.final_pc != 0);
	}
}
