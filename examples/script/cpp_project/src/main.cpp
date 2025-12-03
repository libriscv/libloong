#include "../libloong_api.hpp"
#include <cstdio>

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

	reset_counter();
	int reset_val = get_counter();

	return (after - initial);  // Should be 3
}

void greet(const std::string& name) {
	log_message("Hello, " + name);
}

int factorial(int n) {
	if (n <= 1) return 1;
	return n * factorial(n - 1);
}

} // extern "C"

// Main function for standalone execution (if needed)
int main() {
	// This main is not used for vmcall, but can be used for testing
	printf("Guest app loaded, ready for vmcalls\n");
	fast_exit(0);
	return 0;
}
