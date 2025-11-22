#include <stdio.h>
#include <string.h>

int main() {
    printf("Testing printf:\n");
    printf("Integer: %d\n", 42);
    printf("String: %s\n", "hello");
    printf("Char: %c\n", 'X');
    printf("Long: %ld\n", 123456789L);
    printf("Hex: 0x%x 0x%X\n", 255, 255);
    return 0;
}

long test_strlen(const char* str) {
	if (strcmp(str, "Hello, World!") != 0) {
		return -1;
	}
	return strlen(str);
}
long test_strcmp(const char* a, const char* b) {
	return strcmp(a, b);
}

asm(".pushsection .text\n"
	".global fast_exit\n"
	".type fast_exit, @function\n"
	"fast_exit:\n"
	"	li.w $a7, 94\n"
	"	syscall 0\n"
	".popsection\n");
