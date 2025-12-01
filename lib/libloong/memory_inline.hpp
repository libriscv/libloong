#pragma once
#include <cstdint>
#include <cstring>
#include "libloong_settings.h"

namespace loongarch {

template <typename T, bool EnableSegReg>
inline T Memory::read(address_t addr) const
{
	if constexpr (LA_MASKED_MEMORY_MASK) {
		if constexpr (LA_MASKED_MEMORY_MASK == UINT32_MAX) {
			addr = (uint32_t)addr;
		} else {
			addr &= LA_MASKED_MEMORY_MASK;
		}
	} else {
		if (LA_UNLIKELY(addr < m_rodata_start || addr >= m_arena_size)) {
			protection_fault(addr, "Read from unmapped memory");
		}
	}
#ifdef LA_ENABLE_ARENA_BASE_REGISTER
#  ifdef __linux__
	if constexpr (sizeof(T) <= 8) {
		// On Linux, use GS-relative addressing for faster access
		T value;
		asm volatile ("mov %%gs:(%1), %0" : "=r"(value) : "r"((uint32_t)addr));
		return value;
	}
#  endif
#endif
	return *reinterpret_cast<const T*>(&m_arena[addr]);
}

template <typename T, bool EnableSegReg>
inline void Memory::write(address_t addr, T value)
{
	if constexpr (LA_MASKED_MEMORY_MASK) {
		if constexpr (LA_MASKED_MEMORY_MASK == UINT32_MAX) {
			addr = (uint32_t)addr;
		} else {
			addr &= LA_MASKED_MEMORY_MASK;
		}
	} else {
		if (LA_UNLIKELY(!is_writable(addr, sizeof(T)))) {
			protection_fault(addr, "Write to read-only memory");
		}
	}
#ifdef LA_ENABLE_ARENA_BASE_REGISTER
#  ifdef __linux__
	if constexpr (sizeof(T) <= 8 && EnableSegReg) {
		// On Linux, use GS-relative addressing for faster access
		asm volatile ("mov %0, %%gs:(%1)" : : "r"(value), "r"((uint32_t)addr));
		return;
	}
#  endif
#endif
	*reinterpret_cast<T*>(&m_arena[addr]) = value;
}

template <typename T>
inline const T* Memory::memarray(address_t addr, size_t count) const
{
	if (LA_UNLIKELY(!is_readable(addr, count * sizeof(T)))) {
		throw MachineException(PROTECTION_FAULT, "Read from unmapped memory", addr);
	}

	return reinterpret_cast<const T*>(&m_arena[addr]);
}

template <typename T>
inline T* Memory::writable_memarray(address_t addr, size_t count)
{
	if (LA_UNLIKELY(!is_writable(addr, count * sizeof(T)))) {
		throw MachineException(PROTECTION_FAULT, "Write to read-only memory", addr);
	}

	return reinterpret_cast<T*>(&m_arena[addr]);
}

} // loongarch
