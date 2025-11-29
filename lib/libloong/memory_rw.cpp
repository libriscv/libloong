#include "memory.hpp"
#include "machine.hpp"

namespace loongarch
{
	template <int W>
	void Memory<W>::copy_to_guest(address_t dest, const void* src, size_t len)
	{
		if (LA_UNLIKELY(!is_writable(dest, len))) {
			throw MachineException(PROTECTION_FAULT, "Write to read-only memory", dest);
		}

		std::memcpy(&m_arena[dest], src, len);
	}

	template <int W>
	void Memory<W>::copy_from_guest(void* dest, address_t src, size_t len) const
	{
		if (LA_UNLIKELY(src < m_rodata_start || src + len >= m_arena_size)) {
			throw MachineException(PROTECTION_FAULT, "Read from unmapped memory", src);
		}

		std::memcpy(dest, &m_arena[src], len);
	}

	template <int W>
	void Memory<W>::memset(address_t addr, uint8_t value, size_t len)
	{
		if (LA_UNLIKELY(!is_writable(addr, len))) {
			throw MachineException(PROTECTION_FAULT, "Write to read-only memory", addr);
		}

		std::memset(&m_arena[addr], value, len);
	}

	template <int W>
	int Memory<W>::memcmp(address_t addr1, address_t addr2, size_t len) const
	{
		if (LA_UNLIKELY(addr1 < m_rodata_start || addr1 + len >= m_arena_size)) {
			throw MachineException(PROTECTION_FAULT, "Read from unmapped memory", addr1);
		}
		if (LA_UNLIKELY(addr2 < m_rodata_start || addr2 + len >= m_arena_size)) {
			throw MachineException(PROTECTION_FAULT, "Read from unmapped memory", addr2);
		}

		return std::memcmp(&m_arena[addr1], &m_arena[addr2], len);
	}

	template <int W>
	void Memory<W>::protection_fault(address_t addr, const char* message)
	{
		throw MachineException(PROTECTION_FAULT, message, addr);
	}

	template <int W>
	void Memory<W>::copy_into_arena_unsafe(address_t dest, const void* src, size_t len)
	{
		if (LA_UNLIKELY(dest + len >= m_arena_size)) {
			throw MachineException(PROTECTION_FAULT, "Write to out-of-bounds memory", dest);
		}
		std::memcpy(&m_arena[dest], src, len);
	}

// Template instantiations
#ifdef LA_32
	template struct Memory<LA32>;
#endif
#ifdef LA_64
	template struct Memory<LA64>;
#endif

} // loongarch
