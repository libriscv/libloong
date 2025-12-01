#pragma once

namespace loongarch
{
	// Inline implementations for CPU methods

#if (defined(__GNUG__) || defined(__clang__)) && defined(__linux__)
	LA_ALWAYS_INLINE
	Machine& CPU::machine() noexcept { return *reinterpret_cast<Machine*> (this); }
	LA_ALWAYS_INLINE
	const Machine& CPU::machine() const noexcept { return *reinterpret_cast<const Machine*> (this); }
#else
	LA_ALWAYS_INLINE
	Machine& CPU::machine() noexcept { return this->m_machine; }
	LA_ALWAYS_INLINE
	const Machine& CPU::machine() const noexcept { return this->m_machine; }
#endif

	// memory() methods moved to cpu.cpp to avoid circular dependency

	inline void CPU::jump(address_t addr) {
		if (LA_UNLIKELY(!is_executable(addr))) {
			trigger_exception(EXECUTION_SPACE_PROTECTION_FAULT, addr);
		}
		this->m_regs.pc = addr;
	}

} // loongarch
