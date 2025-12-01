#pragma once

namespace loongarch
{
	// Inline implementations for CPU methods

#if (defined(__GNUG__) || defined(__clang__)) && defined(__linux__)
	template <int W> LA_ALWAYS_INLINE
	Machine<W>& CPU<W>::machine() noexcept { return *reinterpret_cast<Machine<W>*> (this); }
	template <int W> LA_ALWAYS_INLINE
	const Machine<W>& CPU<W>::machine() const noexcept { return *reinterpret_cast<const Machine<W>*> (this); }
#else
	template <int W> LA_ALWAYS_INLINE
	Machine<W>& CPU<W>::machine() noexcept { return this->m_machine; }
	template <int W> LA_ALWAYS_INLINE
	const Machine<W>& CPU<W>::machine() const noexcept { return this->m_machine; }
#endif

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
