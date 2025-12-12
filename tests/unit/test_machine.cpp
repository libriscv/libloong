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

		{
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
		TestMachine machine3(binary);
		machine3.setup_linux();
		auto result3 = machine3.execute();
		REQUIRE(result3.success);
	}

	SECTION("Dead execute segment originator") {
		auto binary = builder.build(R"(
			int main() {
				return 0;
			}
		)", "multi_machine");

		std::unique_ptr<TestMachine> machine1(new TestMachine(binary));
		{
			TestMachine machine2(binary);

			machine1->setup_linux();
			machine2.setup_linux();

			// Both should work independently
			auto result1 = machine1->execute();
			machine1.reset(); // Destroy machine1 before executing machine2
			auto result2 = machine2.execute();

			REQUIRE(result1.success);
			REQUIRE(result2.success);
		}
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

TEST_CASE("System call argument helpers", "[machine][syscall][sysargs]") {
	CodeBuilder builder;

	SECTION("sysarg - integer arguments") {
		auto binary = builder.build(R"(
			int main() { return 0; }
		)", "sysarg_int");

		TestMachine machine(binary);
		machine.setup_linux();

		// Test data for verification
		static int test_arg0 = 0, test_arg2 = 0;
		static long test_arg1 = 0;

		machine.machine().install_syscall_handler(500, [](Machine& m) {
			test_arg0 = m.sysarg<int>(0);
			test_arg1 = m.sysarg<long>(1);
			test_arg2 = m.sysarg<unsigned>(2);
			m.set_result<int>(0);
		});

		// Manually set up registers and call the syscall
		machine.machine().cpu.reg(REG_A0) = 42;
		machine.machine().cpu.reg(REG_A1) = static_cast<uint64_t>(-123);
		machine.machine().cpu.reg(REG_A2) = 999;

		machine.machine().system_call(500);

		REQUIRE(test_arg0 == 42);
		REQUIRE(test_arg1 == -123);
		REQUIRE(test_arg2 == 999);
	}

	SECTION("sysargs - multiple arguments") {
		auto binary = builder.build(R"(
			int main() { return 0; }
		)", "sysargs_multi");

		TestMachine machine(binary);
		machine.setup_linux();

		static bool test_passed = false;

		machine.machine().install_syscall_handler(501, [](Machine& m) {
			auto [a, b, c] = m.sysargs<int, long, unsigned>();
			test_passed = (a == 10 && b == -20 && c == 30);
			m.set_result<int>(0);
		});

		// Set up registers
		machine.machine().cpu.reg(REG_A0) = 10;
		machine.machine().cpu.reg(REG_A1) = static_cast<uint64_t>(-20);
		machine.machine().cpu.reg(REG_A2) = 30;

		machine.machine().system_call(501);
		REQUIRE(test_passed);
	}

	SECTION("sysargs - string argument") {
		auto binary = builder.build(R"(
			char buffer[32];
			int main() { return 0; }
		)", "sysargs_string");

		TestMachine machine(binary);
		machine.setup_linux();

		// Get address of buffer and write a test string to it
		const char* test_str = "Hello, World!";
		uint64_t str_addr = machine.address_of("buffer");
		REQUIRE(str_addr != 0);
		machine.machine().memory.copy_to_guest(str_addr, test_str, strlen(test_str) + 1);

		static bool test_passed = false;

		machine.machine().install_syscall_handler(502, [](Machine& m) {
			auto [str] = m.sysargs<std::string>();
			test_passed = (str == "Hello, World!");
			m.set_result<int>(0);
		});

		// Set up registers
		machine.machine().cpu.reg(REG_A0) = str_addr;

		machine.machine().system_call(502);
		REQUIRE(test_passed);
	}

	SECTION("sysargs - string_view argument") {
		auto binary = builder.build(R"(
			char buffer[32];
			int main() { return 0; }
		)", "sysargs_strview");

		TestMachine machine(binary);
		machine.setup_linux();

		// Get address of buffer and write a test string to it
		const char* test_str = "Test String";
		uint64_t str_addr = machine.address_of("buffer");
		REQUIRE(str_addr != 0);
		machine.machine().memory.copy_to_guest(str_addr, test_str, strlen(test_str));

		static bool test_passed = false;

		machine.machine().install_syscall_handler(503, [](Machine& m) {
			auto [view] = m.sysargs<std::string_view>();
			test_passed = (view == "Test String" && view.size() == 11);
			m.set_result<int>(0);
		});

		// Set up registers (address and length)
		machine.machine().cpu.reg(REG_A0) = str_addr;
		machine.machine().cpu.reg(REG_A1) = 11;  // length

		machine.machine().system_call(503);
		REQUIRE(test_passed);
	}

	SECTION("sysargs - mixed types") {
		auto binary = builder.build(R"(
			char buffer[32];
			int main() { return 0; }
		)", "sysargs_mixed");

		TestMachine machine(binary);
		machine.setup_linux();

		// Get address of buffer and write a test string to it
		const char* test_str = "Mixed";
		uint64_t str_addr = machine.address_of("buffer");
		REQUIRE(str_addr != 0);
		machine.machine().memory.copy_to_guest(str_addr, test_str, strlen(test_str) + 1);

		static bool test_passed = false;

		machine.machine().install_syscall_handler(504, [](Machine& m) {
			auto [num, str, flag] = m.sysargs<int, std::string, bool>();
			test_passed = (num == 42 && str == "Mixed" && flag == true);
			m.set_result<int>(0);
		});

		// Set up registers
		machine.machine().cpu.reg(REG_A0) = 42;
		machine.machine().cpu.reg(REG_A1) = str_addr;
		machine.machine().cpu.reg(REG_A2) = 1;

		machine.machine().system_call(504);
		REQUIRE(test_passed);
	}
}

TEST_CASE("Exception handling", "[machine][exceptions]") {
	CodeBuilder builder;

	SECTION("Exception from system call") {
		auto binary = builder.build(R"(
			int trigger_exception() {
				register int a7 __asm__("a7") = 500; // Syscall number
				__asm__ volatile ("syscall 0" : : "r"(a7) : "memory");
				return 1234; // Should not reach here
			}

			static int call = 0;
			int main() {
				if (call) {
					return trigger_exception();
				}
				return 0;
			}
		)", "syscall_exception");

		TestMachine machine(binary);
		machine.setup_linux();

		static bool was_called = false;
		machine.machine().install_syscall_handler(500, [](Machine& m) {
			was_called = true;
			throw MachineException(GUEST_ABORT, "Test exception");
		});

		// In JIT-mode there is no unwinding, so we can use
		// the std::exception_ptr stored in the machine
		// otherwise, we expect a MachineException to be thrown
		try {
			machine.vmcall("trigger_exception");
			FAIL("Expected exception was not thrown");
		} catch (const MachineException& e) {
			REQUIRE(std::string(e.what()).find("Test exception") != std::string::npos);
			REQUIRE(machine.machine().has_current_exception() == false);
		}
		REQUIRE(was_called);
		was_called = false;

		try {
			// With execution timeout
			machine.machine().vmcall<int, 10000ull>("trigger_exception");
			FAIL("Expected exception was not thrown");
		} catch (const MachineException& e) {
			REQUIRE(std::string(e.what()).find("Test exception") != std::string::npos);
			REQUIRE(machine.machine().has_current_exception() == false);
		}
		REQUIRE(was_called);
	}
}
