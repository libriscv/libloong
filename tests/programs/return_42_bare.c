// Bare metal return 42 - no libc
// This directly calls exit syscall

__asm__(
    ".globl _start\n"
    "_start:\n"
    "    li.w $a0, 42\n"
    "    li.w $a7, 93\n"
    "    syscall 0\n"
);
