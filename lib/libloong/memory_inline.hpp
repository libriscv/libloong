#pragma once
#include <cstdint>
#include <cstring>

namespace loongarch {

template <int W>
template <typename T>
inline T Memory<W>::read(address_t addr) const
{
	if (LA_UNLIKELY(addr < m_rodata_start || addr >= m_arena_size)) {
		protection_fault(addr, "Read from unmapped memory");
	}

	const size_t offset = addr - m_rodata_start;
	return *reinterpret_cast<const T*>(&m_arena[offset]);
}

template <int W>
template <typename T>
inline void Memory<W>::write(address_t addr, T value)
{
	if (LA_UNLIKELY(!is_writable(addr, sizeof(T)))) {
		protection_fault(addr, "Write to read-only memory");
	}

	const size_t offset = addr - m_rodata_start;
	*reinterpret_cast<T*>(&m_arena[offset]) = value;
}

template <int W>
template <typename T>
inline const T* Memory<W>::memarray(address_t addr, size_t count) const
{
	if (LA_UNLIKELY(addr < m_rodata_start || addr + count * sizeof(T) >= m_arena_size)) {
		throw MachineException(PROTECTION_FAULT, "Read from unmapped memory", addr);
	}

	const size_t offset = addr - m_rodata_start;
	return reinterpret_cast<const T*>(&m_arena[offset]);
}

template <int W>
template <typename T>
inline T* Memory<W>::writable_memarray(address_t addr, size_t count)
{
	if (LA_UNLIKELY(!is_writable(addr, count * sizeof(T)))) {
		throw MachineException(PROTECTION_FAULT, "Write to read-only memory", addr);
	}

	const size_t offset = addr - m_rodata_start;
	return reinterpret_cast<T*>(&m_arena[offset]);
}

} // loongarch
