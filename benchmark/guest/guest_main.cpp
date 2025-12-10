#include <cstdlib>
// Simple guest program for benchmarking libloong vmcall overhead
// This program provides minimal test functions with various argument counts

// Fast exit function for vmcalls - returns immediately to host
asm(".pushsection .text\n"
	".global fast_exit\n"
	".type fast_exit, @function\n"
	"fast_exit:\n"
	"	move $zero, $zero\n"  // Indicate fast exit
	".popsection\n");

extern "C" {

// Empty function - measures pure vmcall overhead with no arguments
void empty_function() {
}

// Functions with different argument counts - for testing argument passing overhead
void test_args_0() {
}

void test_args_1(int a) {
}

void test_args_2(int a, int b) {
}

void test_args_3(int a, int b, int c) {
}

void test_args_4(int a, int b, int c, int d) {
}

void test_args_5(int a, int b, int c, int d, int e) {
}

void test_args_6(int a, int b, int c, int d, int e, int f) {
}

void test_args_7(int a, int b, int c, int d, int e, int f, int g) {
}

void test_args_8(int a, int b, int c, int d, int e, int f, int g, int h) {
}

void test_syscall_0() {
	static constexpr int syscall_number = 1;
	asm ("syscall %0" :: "I"(syscall_number));
}

void test_syscall_1(int a) {
	static constexpr int syscall_number = 1;
	register int a0 asm("a0") = a;

	asm ("syscall %0" :: "I"(syscall_number), "r"(a0));
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

void test_heap(int size) {
	void* arr = malloc(size);
	asm("" ::: "memory"); // Prevent optimization
	free(arr);
}

void test_heap_cxx(int size) {
	int* arr = new int[size];
	asm("" ::: "memory"); // Prevent optimization
	delete[] arr;
}

static long fib(long n, long acc, long prev)
{
	if (n == 0)
		return acc;
	else
		return fib(n - 1, prev + acc, acc);
}

long test_fibonacci(long n) {
	return fib(n, 0, 1);
}

// Main function - does nothing in benchmark context
int main() {
	return 0;
}

} // extern "C"
