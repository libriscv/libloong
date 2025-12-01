// Simple guest program for benchmarking libloong vmcall overhead
// This program provides minimal test functions with various argument counts

// Fast exit function for vmcalls - returns immediately to host
asm(".pushsection .text\n"
	".global fast_exit\n"
	".type fast_exit, @function\n"
	"fast_exit:\n"
	"	li.w $a7, 94\n"    // syscall number for exit_group
	"	syscall 0\n"
	".popsection\n");

extern "C" {

// Empty function - measures pure vmcall overhead with no arguments
void empty_function() {
}

// Functions with different argument counts - for testing argument passing overhead
void test_args_0() {
}

int test_args_1(int a) {
	return a;
}

int test_args_2(int a, int b) {
	return a + b;
}

int test_args_3(int a, int b, int c) {
	return a + b + c;
}

int test_args_4(int a, int b, int c, int d) {
	return a + b + c + d;
}

int test_args_5(int a, int b, int c, int d, int e) {
	return a + b + c + d + e;
}

int test_args_6(int a, int b, int c, int d, int e, int f) {
	return a + b + c + d + e + f;
}

int test_args_7(int a, int b, int c, int d, int e, int f, int g) {
	return a + b + c + d + e + f + g;
}

int test_args_8(int a, int b, int c, int d, int e, int f, int g, int h) {
	return a + b + c + d + e + f + g + h;
}

// Simple computation - for testing actual work overhead
int simple_computation(int n) {
	int result = 0;
	for (int i = 0; i < n; i++) {
		result += i;
	}
	return result;
}

// Fibonacci - for testing recursive call overhead
int fibonacci(int n) {
	if (n <= 1)
		return n;
	return fibonacci(n - 1) + fibonacci(n - 2);
}

// Main function - does nothing in benchmark context
int main() {
	return 0;
}

} // extern "C"
