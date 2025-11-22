#include "machine.hpp"

namespace loongarch
{
	// Serialization stubs - to be implemented

	// Template instantiations
#ifdef LA_32
	template struct Machine<LA32>;
#endif
#ifdef LA_64
	template struct Machine<LA64>;
#endif

} // namespace loongarch
