#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>
extern "C" void fast_exit(int code);

int main() {
	std::vector<std::thread> threads;
	for (int i = 0; i < 4; ++i)
		threads.emplace_back([]{
			printf("Hello from LoongArch C++20 std::thread 0x%X!\n",
				std::this_thread::get_id());
		});
	for (auto& t : threads)
		t.join();
	printf("All threads joined, exiting main.\n");
	fflush(stdout);
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
