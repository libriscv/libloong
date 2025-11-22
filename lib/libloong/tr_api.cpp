#include "machine.hpp"

namespace loongarch
{
	// Binary translation API stubs

	// Template instantiations
#ifdef LA_32
	template struct Machine<LA32>;
#endif
#ifdef LA_64
	template struct Machine<LA64>;
#endif

} // namespace loongarch
