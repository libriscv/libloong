#include "machine.hpp"

#include <cstdio>
#ifdef __linux__
#include <cxxabi.h>
#endif

namespace loongarch {

std::string Machine::lookup_demangled_symbol(address_t addr, bool with_offset) const
{
	const Symbol* symbol = lookup_symbol(addr);
	if (!symbol) {
		return {};
	}
	std::string result;
#ifdef __linux__
	int status = 0;
	char* demangled = abi::__cxa_demangle(symbol->name.c_str(), nullptr, nullptr, &status);
	if (status == 0 && demangled) {
		result = demangled;
		std::free(demangled);
	} else {
		result = symbol->name;
	}
#else
	result = symbol->name;
#endif
	if (with_offset) {
		address_t offset = addr - symbol->address;
		if (offset != 0) {
			char buffer[33];
			std::snprintf(buffer, sizeof(buffer), "+0x%lx", (unsigned long)offset);
			result += buffer;
		}
	}
	return result;
}

// This is a remote backtrace, so we obviously cannot use native stack unwinding.
// Instead, we will simulate a backtrace by walking the saved return addresses
// on the stack. This is inherently unreliable, but it's better than nothing.
std::string Machine::backtrace(address_t initial) const
{
	if (initial == 0) {
		initial = cpu.pc();
	}
	char buffer[4096];
	int n = snprintf(buffer, sizeof(buffer),
		"#-: 0x%016lx %s\n",
		(unsigned long)initial,
		lookup_demangled_symbol(initial).c_str());

	std::string result;
	if (n > 0) {
		result.append(buffer, static_cast<size_t>(n));
	} else {
		return result;
	}

	/// XXX: If the binary has unwinding information, we could use that here
	/// to do a more reliable backtrace. For now, we will just read the return
	/// addresses from the stack.
	address_t sp = cpu.reg(REG_SP);
	address_t ra = cpu.reg(REG_RA);
	size_t depth = 0;
	while (ra != 0 && depth < 64) {
		int n = snprintf(buffer, sizeof(buffer),
			"#%zu: 0x%016lx %s\n",
			depth,
			(unsigned long)ra,
			lookup_demangled_symbol(ra).c_str());
		if (n > 0) {
			result.append(buffer, static_cast<size_t>(n));
		} else {
			break;
		}
		// Read next return address from stack
		try {
			ra = memory.template read<address_t>(sp);
			sp += sizeof(address_t);
		} catch (...) {
			break; // Unable to read memory
		}
		depth++;
	}
	return result;
}

} // loongarch
