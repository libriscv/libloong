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

		const size_t offset = dest - m_rodata_start;
		std::memcpy(&m_arena[offset], src, len);
	}

	template <int W>
	void Memory<W>::copy_from_guest(void* dest, address_t src, size_t len) const
	{
		if (LA_UNLIKELY(src < m_rodata_start || src + len >= m_arena_size)) {
			throw MachineException(PROTECTION_FAULT, "Read from unmapped memory", src);
		}

		const size_t offset = src - m_rodata_start;
		std::memcpy(dest, &m_arena[offset], len);
	}

	template <int W>
	void Memory<W>::memset(address_t addr, uint8_t value, size_t len)
	{
		if (LA_UNLIKELY(!is_writable(addr, len))) {
			throw MachineException(PROTECTION_FAULT, "Write to read-only memory", addr);
		}

		const size_t offset = addr - m_rodata_start;
		std::memset(&m_arena[offset], value, len);
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

		return std::memcmp(&m_arena[addr1 - m_rodata_start], &m_arena[addr2 - m_rodata_start], len);
	}

	template <int W>
	void Memory<W>::protection_fault(address_t addr, const char* message)
	{
		throw MachineException(PROTECTION_FAULT, message, addr);
	}

// Template instantiations
#ifdef LA_32
	template struct Memory<LA32>;
#endif
#ifdef LA_64
	template struct Memory<LA64>;
#endif

} // loongarch
