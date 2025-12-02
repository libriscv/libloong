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
	register int a7 asm("a7") = 1; // our custom syscall

	asm ("syscall 0" :: "r"(a7));
}

void test_syscall_1(int a) {
	register int a7 asm("a7") = 1; // our custom syscall
	register int a0 asm("a0") = a;

	asm ("syscall 0" :: "r"(a7), "r"(a0));
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

// Main function - does nothing in benchmark context
int main() {
	return 0;
}

} // extern "C"
