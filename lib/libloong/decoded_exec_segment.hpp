#pragma once
#include "common.hpp"
#include "decoder_cache.hpp"

namespace loongarch
{
	template <int W>
	struct DecodedExecuteSegment {
		using address_t = address_type<W>;

		DecodedExecuteSegment(address_t begin, address_t end, size_t pages)
			: m_exec_begin(begin), m_exec_end(end), m_pages(pages) {}

		~DecodedExecuteSegment() {
			if (m_decoder_cache.cache) {
				delete[] m_decoder_cache.cache;
			}
		}

		bool is_within(address_t addr, size_t len = 4) const noexcept {
			return addr >= m_exec_begin && addr + len <= m_exec_end;
		}

		address_t exec_begin() const noexcept { return m_exec_begin; }
		address_t exec_end() const noexcept { return m_exec_end; }

		auto* decoder_cache() noexcept { return m_decoder_cache.cache; }
		auto* decoder_cache() const noexcept { return m_decoder_cache.cache; }
		size_t decoder_cache_size() const noexcept { return m_decoder_cache.size; }

		void set_decoder_cache(DecoderData<W>* cache, size_t size) noexcept {
			m_decoder_cache.cache = cache;
			m_decoder_cache.size = size;
		}

		size_t size_bytes() const noexcept { return m_exec_end - m_exec_begin; }
		bool empty() const noexcept { return m_exec_begin >= m_exec_end; }

		void set(address_t entry_addr, const DecoderData<W>& data);

		bool is_stale() const noexcept { return m_stale; }
		void set_stale(bool stale) noexcept { m_stale = stale; }

		bool is_execute_only() const noexcept { return m_execute_only; }

		uint32_t optimize_bytecode(uint8_t& bytecode, address_t pc, uint32_t instruction_bits) const;

	private:
		address_t m_exec_begin;
		address_t m_exec_end;
		size_t m_pages;
		DecoderCache<W> m_decoder_cache;
		bool m_stale = false;
		bool m_execute_only = false;
	};

} // loongarch
