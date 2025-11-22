#pragma once

namespace loongarch
{
	// Inline implementations for CPU methods

	template <int W>
	inline Memory<W>& CPU<W>::memory() noexcept {
		return machine().memory;
	}

	template <int W>
	inline const Memory<W>& CPU<W>::memory() const noexcept {
		return machine().memory;
	}

	template <int W>
	inline void CPU<W>::jump(address_t addr) {
		if (LA_UNLIKELY(!is_executable(addr))) {
			trigger_exception(EXECUTION_SPACE_PROTECTION_FAULT, addr);
		}
		this->m_regs.pc = addr;
	}

} // loongarch
