#pragma once
#include "common.hpp"
#include "decoder_cache.hpp"

namespace loongarch
{
	// Forward declaration
	template <int W>
	void populate_decoder_cache(DecodedExecuteSegment<W>& segment, const uint8_t* code, size_t code_size);

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

		const uint8_t* exec_data(address_t offset = 0) const noexcept {
			return reinterpret_cast<const uint8_t*>(m_pagedata_base + offset);
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

		bool is_stale() const noexcept { return m_stale; }
		void set_stale(bool stale) noexcept { m_stale = stale; }

		bool is_execute_only() const noexcept { return m_execute_only; }

	private:
		address_t m_exec_begin;
		address_t m_exec_end;
		address_t m_pagedata_base;
		size_t m_pages;
		DecoderCache<W> m_decoder_cache;
		bool m_stale = false;
		bool m_execute_only = false;
	};

} // loongarch
