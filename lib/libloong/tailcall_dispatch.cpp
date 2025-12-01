#include "cpu.hpp"
#include "machine.hpp"

namespace loongarch
{
	bool CPU::simulate(address_t pc, uint64_t icounter, uint64_t maxcounter)
	{
		throw std::runtime_error("Tailcall dispatch not implemented");
	}

	void CPU::simulate_inaccurate(address_t pc)
	{
		throw std::runtime_error("Inaccurate tailcall dispatch not implemented");
	}

	// Template instantiations
// Removed: 	template bool CPU::simulate(address_t, uint64_t, uint64_t);
// Removed: 	template void CPU::simulate_inaccurate(address_t);
#endif

} // namespace loongarch
