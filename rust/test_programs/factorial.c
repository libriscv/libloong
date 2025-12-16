#include <stdio.h>

unsigned long factorial(unsigned long n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

int main() {
    printf("factorial(5) = %lu\n", factorial(5));
    return 0;
}
