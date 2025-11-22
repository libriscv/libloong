#include <cstdio>
#include <cstring>

int main() {
    return 0;
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
