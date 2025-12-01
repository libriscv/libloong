#pragma once
#include "../machine.hpp"

namespace loongarch
{
	// Setup POSIX-like threading syscalls
	// This provides basic threading support for single-threaded emulation
	void setup_posix_threads(Machine& machine);

} // namespace loongarch
