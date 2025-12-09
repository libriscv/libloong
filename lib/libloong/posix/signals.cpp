#include "signals.hpp"
#include "../machine.hpp"
#include <stdexcept>

namespace loongarch
{
	SignalAction& Signals::get(int sig)
	{
		if (sig == 0) {
			return signals[0];
		}
		else if (sig < 1 || sig > 64) {
			throw MachineException(INVALID_PROGRAM,
				"Signal number out of range", sig);
		}
		return signals.at(sig-1);
	}

	void Signals::enter(Machine& machine, int sig)
	{
		// Ignore signal 0
		if (sig == 0)
			return;

		auto& sigact = this->get(sig);

		// Switch to alternate stack if configured
		if (sigact.altstack) {
			auto& stack = per_thread(machine.gettid()).stack;
			machine.cpu.reg(REG_SP) = stack.ss_sp + stack.ss_size;
		}

		// Jump to handler
		machine.cpu.jump(sigact.handler-4);
	}

} // namespace loongarch
