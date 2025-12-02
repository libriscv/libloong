#pragma once

#include <cstdint>

namespace loongarch {

struct InstrCounter {
	InstrCounter(uint64_t icounter, uint64_t maxcounter)
		: m_counter(icounter), m_max(maxcounter) {}

	uint64_t value() const noexcept { return m_counter; }
	uint64_t max() const noexcept { return m_max; }
	void stop() noexcept { m_max = 0; }
	void increment_counter(uint64_t cnt) { m_counter += cnt; }
	bool overflowed() const noexcept { return m_counter >= m_max; }

	template<typename MachineType>
	void apply(MachineType& machine) {
		machine.set_instruction_counter(m_counter);
		machine.set_max_instructions(m_max);
	}

	template<typename MachineType>
	void retrieve_max_counter(MachineType& machine) {
		m_max = machine.max_instructions();
	}

	template<typename MachineType>
	void retrieve_counters(MachineType& machine) {
		m_counter = machine.instruction_counter();
		m_max = machine.max_instructions();
	}

private:
	uint64_t m_counter;
	uint64_t m_max;
};

} // namespace loongarch
