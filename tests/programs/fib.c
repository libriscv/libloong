#include <stdint.h>

static long fib(long n, long acc, long prev)
{
	if (n == 0)
		return acc;
	else
		return fib(n - 1, prev + acc, acc);
}

static void sys_exit(long code) {
	register long a0 __asm__("a0") = code;
	register long a7 __asm__("a7") = 93; // sys_exit
	__asm__ volatile ("syscall 0"
		: "+r"(a0)
		: "r"(a7)
		: "memory");
	__builtin_unreachable();
}

int main() {
	const volatile long n = 256000000ll;
	sys_exit(fib(n, 0, 1));
}
