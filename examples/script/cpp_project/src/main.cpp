#include "../libloong_api.hpp"
#include <fmt/core.h>

// Example guest functions that can be called from host

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

int test_counter() {
	int initial = get_counter();
	increment_counter();
	increment_counter();
	increment_counter();
	int after = get_counter();
	fmt::print("  [GUEST] Counter: initial = {}, after = {}\n", initial, after);
	fflush(stdout);

	reset_counter();
	int reset_val = get_counter();

	return (after - initial);  // Should be 3
}

void greet(const std::string& name) {
	log_message(fmt::format("Hello, {}!", name));
}

int factorial(int n) {
	if (n <= 1) return 1;
	return n * factorial(n - 1);
}

int test_string_operations() {
	std::string test_str = "Hello, LoongScript!";
	int len = string_length(test_str);
	return len; // Should return 19
}

int test_vector_operations() {
	std::vector<int> numbers = {10, 20, 30, 40, 50};
	print_vector_sum(numbers);
	return static_cast<int>(numbers.size()); // Should return 5
}

// New functions that accept strings and vectors from host via vmcall
int process_message(const std::string& msg) {
	log_message(fmt::format("Processing message: {}", msg));
	return static_cast<int>(msg.length());
}

int sum_numbers(const std::vector<int>& numbers) {
	int sum = 0;
	for (int num : numbers) {
		sum += num;
	}
	return sum;
}

void process_dialogue(const std::string& speaker, const std::vector<int>& scores) {
	log_message(fmt::format("Speaker: {}", speaker));
	for (auto& score : scores) {
		log_message(fmt::format("  Score: {}", score));
	}
	print_vector_sum(scores);
}

// Example 7: Complex nested datatypes
struct Dialogue {
	std::string speaker;
	std::vector<std::string> lines;
};

void do_dialogue(const Dialogue& dlg) {
	log_message(fmt::format("Dialogue by: {}", dlg.speaker));
	for (const auto& line : dlg.lines) {
		log_message(fmt::format("  {}", line));
	}
}

} // extern "C"

// Main function for standalone execution (if needed)
int main() {
	fmt::print(">>> Hello from the LoongScript Guest!\n");
	fflush(stdout);
	fast_exit(0);
}
