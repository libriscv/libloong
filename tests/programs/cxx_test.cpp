#include <cstdio>
#include <cstring>
extern "C" void fast_exit(int code);

int main() {
	// If we don't return from main(), we can
	// throw exceptions from VM function calls.
	fast_exit(42);
}

extern "C" long test_exception() {
	try {
		throw 42;
	} catch (int e) {
		return e;
	}
	return 0;
}

asm(".pushsection .text\n"
	".global fast_exit\n"
	".type fast_exit, @function\n"
	"fast_exit:\n"
	"	li.w $a7, 94\n"
	"	syscall 0\n"
	".popsection\n");
