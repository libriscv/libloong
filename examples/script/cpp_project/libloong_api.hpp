#pragma once

#include <string>
#include <vector>


extern "C" {
    int get_random();
    void increment_counter();
    int get_counter();
    void reset_counter();
    float host_sqrt(float x);
    void host_print(int value);
    void log_message(const std::string& msg);
    int host_add(int a, int b);
}

asm(".pushsection .text\n"
    ".global get_random\n"
    ".type get_random, @function\n"
    "get_random:\n"
    "  ret\n"
    ".popsection\n");
asm(".pushsection .text\n"
    ".global increment_counter\n"
    ".type increment_counter, @function\n"
    "increment_counter:\n"
    "  ret\n"
    ".popsection\n");
asm(".pushsection .text\n"
    ".global get_counter\n"
    ".type get_counter, @function\n"
    "get_counter:\n"
    "  ret\n"
    ".popsection\n");
asm(".pushsection .text\n"
    ".global reset_counter\n"
    ".type reset_counter, @function\n"
    "reset_counter:\n"
    "  ret\n"
    ".popsection\n");
asm(".pushsection .text\n"
    ".global host_sqrt\n"
    ".type host_sqrt, @function\n"
    "host_sqrt:\n"
    "  ret\n"
    ".popsection\n");
asm(".pushsection .text\n"
    ".global host_print\n"
    ".type host_print, @function\n"
    "host_print:\n"
    "  ret\n"
    ".popsection\n");
asm(".pushsection .text\n"
    ".global log_message\n"
    ".type log_message, @function\n"
    "log_message:\n"
    "  ret\n"
    ".popsection\n");
asm(".pushsection .text\n"
    ".global host_add\n"
    ".type host_add, @function\n"
    "host_add:\n"
    "  ret\n"
    ".popsection\n");


// Fast exit for vmcall support
__asm__(
	".pushsection .text\n"
	".global fast_exit\n"
	".type fast_exit, @function\n"
	"fast_exit:\n"
	"  move $zero, $zero\n"
	".popsection\n"
);
extern "C" __attribute__((noreturn)) void fast_exit(int code);
