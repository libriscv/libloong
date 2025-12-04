#include "script.hpp"
#include "host_bindings.hpp"
#include "api_generator.hpp"
#include <fmt/core.h>
#include <cmath>
#include <cassert>
#include <filesystem>

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

// Example 3: String handling host functions (C++ versions)
namespace {
	void init_string_functions() {
		HostBindings::register_function(
			"void log_message(const std::string& msg)",
			[](loongarch::Machine& machine, const loongarch::GuestStdString* msg) {
				fmt::print("  [LOG] {}\n", msg->to_view(machine));
			}
		);

		HostBindings::register_function(
			"int string_length(const std::string& str)",
			[](loongarch::Machine& machine, const loongarch::GuestStdString* str) -> int {
				int len = static_cast<int>(str->size);
				fmt::print("  [HOST] string_length() = {}\n", len);
				return len;
			}
		);

		HostBindings::register_function(
			"void print_vector_sum(const std::vector<int>& vec)",
			[](loongarch::Machine& machine, const loongarch::GuestStdVector<int>* vec) {
				int sum = 0;
				const int* arr = vec->as_array(machine);
				for (size_t i = 0; i < vec->size(); i++) {
					sum += arr[i];
				}
				fmt::print("  [HOST] print_vector_sum({} elements) = {}\n", vec->size(), sum);
			}
		);
	}

	static struct StringFunctionsInit {
		StringFunctionsInit() { init_string_functions(); }
	} string_functions_init;
}

// Example 3b: String handling host functions (Rust versions)
namespace {
	void init_rust_string_functions() {
		HostBindings::register_function(
			"void rust_log_message(const std::string& msg)",
			[](loongarch::Machine& machine, const loongarch::GuestRustString* msg) {
				try {
					fmt::print("  [LOG] {}\n", msg->to_view(machine));
				} catch (const std::exception& e) {
					fmt::print("  [ERROR] Failed to read string: {}\n", e.what());
				}
			});

		HostBindings::register_function(
			"int rust_string_length(const std::string& str)",
			[](loongarch::Machine& machine, const loongarch::GuestRustString* str) -> int {
				int len = static_cast<int>(str->len);
				fmt::print("  [HOST] rust_string_length() = {}\n", len);
				return len;
			});

		HostBindings::register_function(
			"void rust_print_vector_sum(const std::vector<int>& vec)",
			[](loongarch::Machine& m, const loongarch::GuestRustVector<int>* vec) {
				int sum = 0;
				const int* arr = vec->as_array(m);
				for (size_t i = 0; i < vec->size(); i++) {
					sum += arr[i];
				}
				fmt::print("  [HOST] rust_print_vector_sum({} elements) = {}\n", vec->size(), sum);
			});
	}

	static struct RustStringFunctionsInit {
		RustStringFunctionsInit() { init_rust_string_functions(); }
	} rust_string_functions_init;
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

int main(int argc, char* argv[]) {
	fmt::print("LoongScript Framework - Project-Based Example\n");
	fmt::print("===================================================\n\n");

	// Check for --generate-bindings flag
	if (argc > 1 && std::string(argv[1]) == "--generate-bindings") {
		fmt::print("Generating API bindings...\n");

		// Generate C++ API
		const std::filesystem::path cpp_api_path = "cpp_project/libloong_api.hpp";
		APIGenerator::write_cpp_api(cpp_api_path);
		fmt::print("  C++ API: {}\n", cpp_api_path.string());

		// Generate Rust API with scanning for #[no_mangle] functions
		const std::filesystem::path rust_api_path = "rust_project/libloong_api.rs";
		const std::filesystem::path rust_project_path = "rust_project/src";
		APIGenerator::write_rust_api(rust_api_path, rust_project_path);
		fmt::print("  Rust API: {} (with DCE protection)\n", rust_api_path.string());

		fmt::print("\nAPI generation complete!\n");
		fmt::print("You can now build the guest projects:\n");
		fmt::print("  C++:  cd cpp_project && chmod +x build.sh && ./build.sh\n");
		fmt::print("  Rust: cd rust_project && chmod +x build.sh && ./build.sh\n");
		return 0;
	}

	// Check for --language flag
	std::string language = "cpp";
	if (argc > 2 && std::string(argv[1]) == "--language") {
		language = argv[2];
	}

	// Determine which guest executable to load
	std::string guest_path;
	if (language == "cpp") {
		guest_path = "cpp_project/guest_app.elf";
	} else if (language == "rust") {
		guest_path = "rust_project/guest_app.elf";
	} else {
		fmt::print(stderr, "Error: Unknown language '{}'. Use 'cpp' or 'rust'.\n", language);
		return 1;
	}

	// Check if guest executable exists
	if (!std::filesystem::exists(guest_path)) {
		fmt::print(stderr, "Error: Guest executable not found: {}\n", guest_path);
		fmt::print(stderr, "Run with --generate-bindings first, then build the guest project.\n");
		return 1;
	}

	fmt::print("Loading {} guest executable: {}\n\n", language, guest_path);

	fmt::print("Example 1: Basic host functions\n");
	try {
		Script script(guest_path);
		script.set_userdata<UserState>(&user_state);

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
		Script script(guest_path);
		script.set_userdata<UserState>(&user_state);

		const int result = script.call<int>("test_counter");
		fmt::print("  test_counter() = {}\n\n", result);
		assert(result == 3);

	} catch (const ScriptException& e) {
		fmt::print(stderr, "  Error: {}\n\n", e.what());
		return 1;
	}

	fmt::print("Example 3: Events with cached function addresses\n");
	try {
		Script script(guest_path);
		script.set_userdata<UserState>(&user_state);

		// Create Event objects - they cache the function address
		Event<int(int)> factorial{script, "factorial"};

		// Call like regular functions - super fast!
		fmt::print("  factorial(5) = {}\n", factorial(5));
		fmt::print("  factorial(7) = {}\n", factorial(7));
		assert(factorial(5) == 120);
		assert(factorial(7) == 5040);
		fmt::print("\n");

	} catch (const ScriptException& e) {
		fmt::print(stderr, "  Error: {}\n\n", e.what());
		return 1;
	}

	fmt::print("Example 4: Multiple Script instances share bindings\n");
	try {
		Script script1(guest_path);
		Script script2(guest_path);

		script1.set_userdata<UserState>(&user_state);
		script2.set_userdata<UserState>(&user_state);

		fmt::print("  Script 1: compute(5, 10) =");
		const int result1 = script1.call<int>("compute", 5, 10);
		fmt::print(" {}\n", result1);

		fmt::print("  Script 2: compute(20, 5) =");
		const int result2 = script2.call<int>("compute", 20, 5);
		fmt::print(" {}\n\n", result2);

		assert(result1 == 15);
		assert(result2 == 25);

	} catch (const ScriptException& e) {
		fmt::print(stderr, "  Error: {}\n\n", e.what());
		return 1;
	}

	fmt::print("Example 5: String and vector handling\n");
	try {
		Script script(guest_path);
		script.set_userdata<UserState>(&user_state);

		fmt::print("  Calling test_string_operations():\n");
		const int str_result = script.call<int>("test_string_operations");
		fmt::print("  Result: {}\n", str_result);

		fmt::print("  Calling test_vector_operations():\n");
		const int vec_result = script.call<int>("test_vector_operations");
		fmt::print("  Result: {}\n\n", vec_result);

	} catch (const ScriptException& e) {
		fmt::print(stderr, "  Error: {}\n\n", e.what());
		return 1;
	}

	fmt::print("All tests passed!\n");
	return 0;
}
