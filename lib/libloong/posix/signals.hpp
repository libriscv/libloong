#pragma once
#include "../common.hpp"
#include "../registers.hpp"
#include <array>
#include <map>

namespace loongarch
{
	// Forward declaration
	struct Machine;

	// Signal stack configuration (per-thread)
	struct SignalStack
	{
		address_t ss_sp = 0x0;      // Stack pointer
		int       ss_flags = 0x0;   // Flags (SS_ONSTACK, SS_DISABLE, etc.)
		address_t ss_size = 0;      // Stack size
	};

	// Signal action configuration (per-signal)
	struct SignalAction
	{
		static constexpr address_t SIG_UNSET = ~address_t(0x0);

		bool is_unset() const noexcept {
			return handler == 0x0 || handler == SIG_UNSET;
		}

		address_t handler = SIG_UNSET;  // Handler address
		bool altstack = false;          // Use alternate stack?
		unsigned mask = 0x0;            // Signal mask
	};

	// Signal return state (stores registers for return)
	struct SignalReturn
	{
		Registers regs;
	};

	// Per-thread signal data
	struct SignalPerThread
	{
		SignalStack  stack;
		SignalReturn sigret;
	};

	// Main signals manager
	struct Signals
	{
		SignalAction& get(int sig);
		void enter(Machine& machine, int sig);

		auto& per_thread(int tid) { return m_per_thread[tid]; }

	private:
		std::array<SignalAction, 64> signals {}; // 64 signals (1-64)
		std::map<int, SignalPerThread> m_per_thread;
	};

} // namespace loongarch
