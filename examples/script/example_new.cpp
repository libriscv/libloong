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

// Example 3: String handling host functions (unified C++ and Rust)
namespace {
	// Template helper to get the correct string type based on language mode
	template<bool IsRust>
	using StringType = std::conditional_t<IsRust, loongarch::GuestRustString, loongarch::GuestStdString>;

	// Template helper to get the correct vector type based on language mode
	template<bool IsRust, typename T>
	using VectorType = std::conditional_t<IsRust, loongarch::GuestRustVector<T>, loongarch::GuestStdVector<T>>;

	// Unified implementation of log_message that works for both C++ and Rust
	template<bool IsRust>
	void log_message_impl(loongarch::Machine& machine, const StringType<IsRust>* msg) {
		try {
			fmt::print("  [LOG] {}\n", msg->to_view(machine));
		} catch (const std::exception& e) {
			fmt::print("  [ERROR] Failed to read string: {}\n", e.what());
		}
	}

	// Unified implementation of string_length that works for both C++ and Rust
	template<bool IsRust>
	int string_length_impl(loongarch::Machine& machine, const StringType<IsRust>* str) {
		int len;
		if constexpr (IsRust) {
			len = static_cast<int>(str->len);
		} else {
			len = static_cast<int>(str->size);
		}
		fmt::print("  [HOST] string_length() = {}\n", len);
		return len;
	}

	// Unified implementation of print_vector_sum that works for both C++ and Rust
	template<bool IsRust>
	void print_vector_sum_impl(loongarch::Machine& machine, const VectorType<IsRust, int>* vec) {
		int sum = 0;
		const int* arr = vec->as_array(machine);
		for (size_t i = 0; i < vec->size(); i++) {
			sum += arr[i];
		}
		fmt::print("  [HOST] print_vector_sum({} elements) = {}\n", vec->size(), sum);
	}

	void init_string_functions() {
		// Register C++ versions
		HostBindings::register_function(
			"void log_message(const std::string& msg)",
			[](loongarch::Machine& machine, const loongarch::GuestStdString* msg) {
				log_message_impl<false>(machine, msg);
			}
		);

		HostBindings::register_function(
			"int string_length(const std::string& str)",
			[](loongarch::Machine& machine, const loongarch::GuestStdString* str) -> int {
				return string_length_impl<false>(machine, str);
			}
		);

		HostBindings::register_function(
			"void print_vector_sum(const std::vector<int>& vec)",
			[](loongarch::Machine& machine, const loongarch::GuestStdVector<int>* vec) {
				print_vector_sum_impl<false>(machine, vec);
			}
		);

		// Register Rust versions
		HostBindings::register_function(
			"void rust_log_message(const std::string& msg)",
			[](loongarch::Machine& machine, const loongarch::GuestRustString* msg) {
				log_message_impl<true>(machine, msg);
			}
		);

		HostBindings::register_function(
			"int rust_string_length(const std::string& str)",
			[](loongarch::Machine& machine, const loongarch::GuestRustString* str) -> int {
				return string_length_impl<true>(machine, str);
			}
		);

		HostBindings::register_function(
			"void rust_print_vector_sum(const std::vector<int>& vec)",
			[](loongarch::Machine& machine, const loongarch::GuestRustVector<int>* vec) {
				print_vector_sum_impl<true>(machine, vec);
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

void print_usage(const char* program_name) {
	fmt::print("Usage: {} [OPTIONS]\n\n", program_name);
	fmt::print("Options:\n");
	fmt::print("  --generate-bindings    Generate API bindings for C++ and Rust guest projects\n");
	fmt::print("  --language <lang>      Specify guest language: 'cpp' or 'rust' (default: cpp)\n");
	fmt::print("  -v, --verbose          Enable verbose output (compilation, patching, warnings)\n");
	fmt::print("  -h, --help             Show this help message\n\n");
	fmt::print("Examples:\n");
	fmt::print("  {}                          # Run with C++ guest\n", program_name);
	fmt::print("  {} --language rust          # Run with Rust guest\n", program_name);
	fmt::print("  {} -v --language cpp        # Run with verbose output\n", program_name);
	fmt::print("  {} --generate-bindings      # Generate API bindings\n", program_name);
}

int main(int argc, char* argv[]) {
	fmt::print("LoongScript Framework - Project-Based Example\n");
	fmt::print("===================================================\n\n");

	// Parse command line arguments
	bool verbose = false;
	bool generate_bindings = false;
	std::string language = "cpp";

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];

		if (arg == "-h" || arg == "--help") {
			print_usage(argv[0]);
			return 0;
		} else if (arg == "-v" || arg == "--verbose") {
			verbose = true;
		} else if (arg == "--generate-bindings") {
			generate_bindings = true;
		} else if (arg == "--language") {
			if (i + 1 < argc) {
				language = argv[++i];
			} else {
				fmt::print(stderr, "Error: --language requires an argument\n\n");
				print_usage(argv[0]);
				return 1;
			}
		} else {
			fmt::print(stderr, "Error: Unknown option '{}'\n\n", arg);
			print_usage(argv[0]);
			return 1;
		}
	}

	// Check for --generate-bindings flag
	if (generate_bindings) {
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

	if (verbose) {
		fmt::print("Verbose mode enabled\n");
	}
	fmt::print("Loading {} guest executable: {}\n\n", language, guest_path);

	// Create ScriptOptions with verbose flag
	ScriptOptions options;
	options.verbose = verbose;

	fmt::print("Example 1: Basic host functions\n");
	try {
		Script script(guest_path, options);
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
		Script script(guest_path, options);
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
		Script script(guest_path, options);
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
		Script script1(guest_path, options);
		Script script2(guest_path, options);

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

	fmt::print("Example 5: String and vector handling with vmcall\n");
	try {
		Script script(guest_path, options);
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

	fmt::print("Example 6: Passing strings and vectors to guest via vmcall\n");
	try {
		Script script(guest_path, options);
		script.set_userdata<UserState>(&user_state);

		// Template lambda to handle both C++ and Rust guests
		auto run_example_6 = [&]<typename StringType, typename VectorType>() {
			// Pass a string to guest function
			fmt::print("  Passing string to guest:\n");
			StringType message{script.machine(), "Hello from host!"};
			Event<int(StringType)> process_msg{script, "process_message"};
			const int len = process_msg(message);
			fmt::print("  Guest returned length: {}\n", len);

			// Pass a vector to guest function
			fmt::print("  Passing vector to guest:\n");
			VectorType numbers{script.machine(), std::vector<int>{10, 20, 30, 40, 50}};
			Event<int(VectorType)> sum_nums{script, "sum_numbers"};
			const int sum = sum_nums(numbers);
			fmt::print("  Guest returned sum: {}\n", sum);

			// Pass both string and vector
			fmt::print("  Passing string and vector together:\n");
			StringType speaker{script.machine(), "Alice"};
			VectorType scores{script.machine(), std::vector<int>{95, 87, 92, 88, 90}};
			Event<void(StringType, VectorType)> process_dlg{script, "process_dialogue"};
			process_dlg(speaker, scores);
		};

		// Dispatch to the appropriate types based on language
		if (language == "cpp") {
			run_example_6.template operator()<loongarch::ScopedCppString, loongarch::ScopedCppVector<int>>();
		} else {
			run_example_6.template operator()<loongarch::ScopedRustString, loongarch::ScopedRustVector<int>>();
		}
		fmt::print("\n");

	} catch (const ScriptException& e) {
		fmt::print(stderr, "  Error: {}\n\n", e.what());
		return 1;
	}

	fmt::print("All tests passed!\n");
	return 0;
}
