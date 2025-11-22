#include "cpu.hpp"
#include "machine.hpp"

namespace loongarch
{
	template <int W>
	bool CPU<W>::simulate(address_t pc, uint64_t icounter, uint64_t maxcounter)
	{
		throw std::runtime_error("Tailcall dispatch not implemented");
	}

	template <int W>
	void CPU<W>::simulate_inaccurate(address_t pc)
	{
		throw std::runtime_error("Inaccurate tailcall dispatch not implemented");
	}

	// Template instantiations
#ifdef LA_32
	template bool CPU<LA32>::simulate(address_type<LA32>, uint64_t, uint64_t);
	template void CPU<LA32>::simulate_inaccurate(address_type<LA32>);
#endif
#ifdef LA_64
	template bool CPU<LA64>::simulate(address_type<LA64>, uint64_t, uint64_t);
	template void CPU<LA64>::simulate_inaccurate(address_type<LA64>);
#endif

} // namespace loongarch
