#pragma once
#include "common.hpp"
#include "decoder_cache.hpp"
#include "tr_types.hpp"
#include <vector>
#include <mutex>
#include <condition_variable>

namespace loongarch
{
	struct DecodedExecuteSegment {
		DecodedExecuteSegment(address_t begin, address_t end)
			: m_exec_begin(begin), m_exec_end(end) {}

		~DecodedExecuteSegment();

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

#ifdef LA_BINARY_TRANSLATION
		// Binary translation support
		bool is_binary_translated() const noexcept { return m_mappings_base_address != nullptr; }
		bool is_libtcc() const noexcept { return m_is_libtcc; }
		void set_libtcc(bool value) noexcept { m_is_libtcc = value; }

		void set_mappings_base_address(const char* addr) noexcept { m_mappings_base_address = addr; }
		const char* mappings_base_address() const noexcept { return m_mappings_base_address; }
		bintr_block_func build_mapping(uint32_t instr_field) const {
			const uintptr_t addr = (uintptr_t)m_mappings_base_address + (uintptr_t)instr_field;
			return reinterpret_cast<bintr_block_func>(addr);
		}

		// Patched decoder cache (for live-patching)
		auto* patched_decoder_cache() noexcept { return m_patched_decoder_cache.cache; }
		auto* patched_decoder_cache() const noexcept { return m_patched_decoder_cache.cache; }
		void set_patched_decoder_cache(DecoderData* cache, size_t size) noexcept {
			m_patched_decoder_cache.cache = cache;
			m_patched_decoder_cache.size = size;
		}

		// Background compilation state
		bool is_background_compiling() const noexcept;
		void set_background_compiling(bool is_bg);
		void wait_for_compilation_complete();

		// Dynamic library handle
		void* bintr_dylib() const noexcept { return m_bintr_dl; }
		void set_bintr_dylib(void* dylib) noexcept { m_bintr_dl = dylib; }
#else
		bool is_binary_translated() const noexcept { return false; }
#endif

	private:
		address_t m_exec_begin;
		address_t m_exec_end;
		DecoderCache m_decoder_cache;
		bool m_stale = false;
		bool m_execute_only = false;
#ifdef LA_BINARY_TRANSLATION
		bool m_is_libtcc = false;
		const char* m_mappings_base_address = nullptr;
		DecoderCache m_patched_decoder_cache;
		void* m_bintr_dl = nullptr;
		mutable std::mutex m_background_compilation_mutex;
		mutable std::condition_variable m_background_compilation_cv;
		bool m_is_background_compiling = false;
#endif
	};

} // loongarch
