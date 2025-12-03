#include "script.hpp"
#include "host_bindings.hpp"
#include <fmt/core.h>
#include <cmath>
#include <cassert>
using namespace loongarch::script;

namespace {
	void init_basic_functions()
	{
		// Register host functions globally at load time
		HostBindings::register_function(
			"int host_add(int a, int b)",
			[](loongarch::Machine&, int a, int b) -> int {
				fmt::print("  [HOST] add({}, {}) called\n", a, b);
				return a + b;
			});

		HostBindings::register_function(
			"void host_print(int value)",
			[](loongarch::Machine&, int value) {
				fmt::print("  [HOST] print({}) called\n", value);
			});

		HostBindings::register_function(
			"float host_sqrt(float x)",
			[](loongarch::Machine&, float x) -> float {
				fmt::print("  [HOST] sqrt({:.2f}) called\n", x);
				return std::sqrt(x);
			});
	}

	static struct BasicFunctionsInit {
		BasicFunctionsInit() { init_basic_functions(); }
	} basic_functions_init;
}

// Example 2: Stateful host functions (can capture state)
namespace {
	static struct UserState {
		int counter = 0;
	} user_state;

	void init_stateful_functions()
	{
		HostBindings::register_function(
			"int get_counter()",
			[](loongarch::Machine& m) -> int {
				UserState& state = *m.get_userdata<Script>()->get_userdata<UserState>();
				fmt::print("  [HOST] get_counter() = {}\n", state.counter);
				return state.counter;
			}
		);

		HostBindings::register_function(
			"void increment_counter()",
			[](loongarch::Machine& m) {
				UserState& state = *m.get_userdata<Script>()->get_userdata<UserState>();
				state.counter++;
				fmt::print("  [HOST] increment_counter(), now = {}\n", state.counter);
			});

		HostBindings::register_function(
			"void reset_counter()",
			[](loongarch::Machine& m) {
				UserState& state = *m.get_userdata<Script>()->get_userdata<UserState>();
				state.counter = 0;
				fmt::print("  [HOST] reset_counter()\n");
			});
	}

	static struct StatefulFunctionsInit {
		StatefulFunctionsInit() { init_stateful_functions(); }
	} stateful_functions_init;
}

// Example 3: String handling host functions
namespace {
	void init_string_functions() {
		HostBindings::register_function(
			"void log_message(const std::string& msg)",
			[](loongarch::Machine& machine, const loongarch::GuestStdString* msg) {
				fmt::print("  [LOG] {}\n", msg->to_view(machine));
			}
		);
	}

	static struct StringFunctionsInit {
		StringFunctionsInit() { init_string_functions(); }
	} string_functions_init;
}

// Example 4: Random number generator
namespace {
	static int random_value = 5;

	void init_random_functions() {
		HostBindings::register_function(
			"int get_random()",
			[](loongarch::Machine&) -> int {
				return random_value++;
			}
		);
	}

	static struct RandomFunctionsInit {
		RandomFunctionsInit() { init_random_functions(); }
	} random_functions_init;
}

int main() {
	fmt::print("LoongScript Framework - Static Binding Example\n");
	fmt::print("===================================================\n\n");

	fmt::print("Example 1: Basic host functions (auto-injected)\n");
	try {
		Script script = Script::from_source(R"(
		extern "C" {
			int compute(int a, int b) {
				host_print(a);
				host_print(b);
				const int sum = host_add(a, b);
				host_print(sum);
				return sum;
			}

			float calculate_area(float radius) {
				const float r2 = radius * radius;
				return 3.14159f * r2;
			}
		})");

		fmt::print("  Calling compute(10, 32):\n");
		const int result = script.call<int>("compute", 10, 32);
		fmt::print("  Result: {}\n", result);
		assert(result == 42);

		fmt::print("  Calling calculate_area(5.0):\n");
		const float area = script.call<float>("calculate_area", 5.0f);
		fmt::print("  Result: {:.2f}\n\n", area);

	} catch (const std::exception& e) {
		fmt::print(stderr, "  Error: {}\n\n", e.what());
		return 1;
	}

	fmt::print("Example 2: Stateful host callbacks\n");
	try {
		Script script = Script::from_source(R"(
		extern "C" {
			int test_counter() {
				int initial = get_counter();
				increment_counter();
				increment_counter();
				increment_counter();
				int after = get_counter();

				reset_counter();
				int reset_val = get_counter();

				return (after - initial);  // Should be 3
			}
		})");
		script.set_userdata<UserState>(&user_state);

		const int result = script.call<int>("test_counter");
		fmt::print("  test_counter() = {}\n\n", result);
		assert(result == 3);

	} catch (const ScriptException& e) {
		fmt::print(stderr, "  Error: {}\n\n", e.what());
		return 1;
	}

	fmt::print("Example 3: std::string handling\n");
	try {
		Script script = Script::from_source(R"(
		extern "C" void greet(const std::string& name) {
			log_message("Hello, " + name);
		})");

		Event<void(loongarch::GuestStdString)> greet_event{script, "greet"};
		loongarch::ScopedCppString name{script.machine(), "World!"};
		greet_event(name);
		fmt::print("\n");

	} catch (const ScriptException& e) {
		fmt::print(stderr, "  Error: {}\n\n", e.what());
		return 1;
	}

	fmt::print("Example 4: Multiple Script instances share bindings\n");
	try {
		Script script1 = Script::from_source(R"(
		extern "C" {
			int process_a(int x) {
				return host_add(x, 10);
			}
		})");

		Script script2 = Script::from_source(R"(
		extern "C" {
			int process_b(int y) {
				return host_add(y, 20);
			}
		})");

		fmt::print("  Script 1: process_a(5) =");
		const int result1 = script1.call<int>("process_a", 5);
		fmt::print(" {}\n", result1);

		fmt::print("  Script 2: process_b(5) =");
		const int result2 = script2.call<int>("process_b", 5);
		fmt::print(" {}\n\n", result2);

		assert(result1 == 15);
		assert(result2 == 25);

	} catch (const ScriptException& e) {
		fmt::print(stderr, "  Error: {}\n\n", e.what());
		return 1;
	}

	fmt::print("Example 5: Events with complex types\n");
	try {
		HostBindings::append_header_content(R"(
			struct Dialogue {
				std::string speaker;
				std::vector<std::string> lines;
			};
		)");
		Script script = Script::from_source(R"(
		extern "C" {
			#include <cstdio>
			int factorial(int n) {
				if (n <= 1) return 1;
				return n * factorial(n - 1);
			}

			void do_dialogue(const Dialogue& dlg) {
				log_message("Dialogue by " + dlg.speaker + ":");
				for (const auto& line : dlg.lines) {
					log_message("  " + line);
				}
			}
		})");

		// Create Event objects - they cache the function address
		Event<int(int)> factorial{script, "factorial"};

		// Call like regular functions - super fast!
		fmt::print("  factorial(5) = {}\n", factorial(5));
		fmt::print("  factorial(7) = {}\n", factorial(7));
		assert(factorial(5) == 120);
		assert(factorial(7) == 5040);
		fmt::print("\n");

		using namespace loongarch;
		struct Dialogue {
			GuestStdString speaker;
			GuestStdVector<GuestStdString> lines;

			Dialogue(Machine& machine, const std::string& spk, const std::vector<std::string>& lns)
				: speaker(machine, spk), lines(machine, lns) {}

			void fix_addresses(Machine& machine, address_t self) {
				speaker.fix_addresses(machine, self + offsetof(Dialogue, speaker));
				lines.fix_addresses(machine, self + offsetof(Dialogue, lines));
			}
		};
		ScopedArenaObject<Dialogue> dlg{script.machine(),
			script.machine(),
			"Alice",
			std::vector<std::string>{
				"Hello there!",
				"Welcome to the LoongScript demo.",
				"Enjoy your stay!"
			}
		};
		Event<void(ScopedArenaObject<Dialogue>)> do_dialogue{script, "do_dialogue"};
		do_dialogue(dlg);
		fmt::print("\n");

	} catch (const ScriptException& e) {
		fmt::print(stderr, "  Error: {}\n\n", e.what());
		return 1;
	}

	return 0;
}
