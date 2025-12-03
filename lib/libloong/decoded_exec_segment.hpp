#pragma once
#include "common.hpp"
#include "decoder_cache.hpp"

namespace loongarch
{
	struct DecodedExecuteSegment {
		DecodedExecuteSegment(address_t begin, address_t end)
			: m_exec_begin(begin), m_exec_end(end) {}

		~DecodedExecuteSegment() {
			if (m_decoder_cache.cache) {
				delete[] m_decoder_cache.cache;
			}
		}

		bool is_within(address_t addr, size_t len = 4) const noexcept {
			address_t addr_end;
#ifdef _MSC_VER
			addr_end = addr + len;
			return addr >= m_exec_begin && addr_end <= m_exec_end && (addr_end > m_exec_begin);
#else
			if (!__builtin_add_overflow(addr, len, &addr_end))
				return addr >= m_exec_begin && addr_end <= m_exec_end && (addr_end > m_exec_begin);
#endif
			return false;
		}

		address_t exec_begin() const noexcept { return m_exec_begin; }
		address_t exec_end() const noexcept { return m_exec_end; }

		auto* decoder_cache() noexcept { return m_decoder_cache.cache; }
		auto* decoder_cache() const noexcept { return m_decoder_cache.cache; }
		auto* pc_relative_decoder_cache(address_t pc = 0) noexcept {
			return m_decoder_cache.cache - exec_begin() / 4 + (pc / 4);
		}
		size_t decoder_cache_size() const noexcept { return m_decoder_cache.size; }

		void set_decoder_cache(DecoderData* cache, size_t size) noexcept {
			m_decoder_cache.cache = cache;
			m_decoder_cache.size = size;
		}

		size_t size_bytes() const noexcept { return m_exec_end - m_exec_begin; }
		bool empty() const noexcept { return m_exec_begin >= m_exec_end; }

		void set(address_t entry_addr, const DecoderData& data);

		bool is_stale() const noexcept { return m_stale; }
		void set_stale(bool stale) noexcept { m_stale = stale; }

		bool is_execute_only() const noexcept { return m_execute_only; }

		uint32_t optimize_bytecode(uint8_t& bytecode, address_t pc, uint32_t instruction_bits) const;

	private:
		address_t m_exec_begin;
		address_t m_exec_end;
		DecoderCache m_decoder_cache;
		bool m_stale = false;
		bool m_execute_only = false;
	};

} // loongarch
