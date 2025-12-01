#pragma once
#include <cstdint>
#include <cstring>
#include "libloong_settings.h"

namespace loongarch {

template <typename T>
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
	return *reinterpret_cast<const T*>(&m_arena[addr]);
}

template <typename T>
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
	*reinterpret_cast<T*>(&m_arena[addr]) = value;
}

template <typename T>
inline const T* Memory::memarray(address_t addr, size_t count) const
{
	if (LA_UNLIKELY(addr < m_rodata_start || addr + count * sizeof(T) >= m_arena_size)) {
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
