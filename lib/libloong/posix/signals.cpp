#include "signals.hpp"
#include "../machine.hpp"
#include <stdexcept>

namespace loongarch
{
	SignalAction& Signals::get(int sig)
	{
		if (sig < 1 || sig > 64) {
			throw MachineException(INVALID_PROGRAM,
				"Signal number out of range", sig);
		}
		return signals[sig-1];
	}

	void Signals::enter(Machine& machine, int sig)
	{
		// Ignore signal 0
		if (sig == 0)
			return;

		auto& sigact = signals.at(sig);

		// Switch to alternate stack if configured
		if (sigact.altstack) {
			printf("Switching to alternate signal stack from 0x%lx for signal %d\n", machine.cpu.reg(REG_SP), sig);
			auto& stack = per_thread(machine.gettid()).stack;
			machine.cpu.reg(REG_SP) = stack.ss_sp + stack.ss_size;
		}

		// Jump to handler
		printf("Jumping to signal handler 0x%lx for signal %d (stack=0x%lx)\n", sigact.handler, sig, machine.cpu.reg(REG_SP));
		machine.cpu.jump(sigact.handler-4);
	}

} // namespace loongarch
