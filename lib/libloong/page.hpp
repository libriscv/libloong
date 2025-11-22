#pragma once
#include "common.hpp"
#include <cstdint>

namespace loongarch
{
	// Memory page structure
	struct Page {
		static constexpr size_t SIZE = LA_PAGE_SIZE;
		
		struct Attributes {
			bool read  : 1;
			bool write : 1;
			bool exec  : 1;
			bool user  : 1;
			uint8_t _unused : 4;

			constexpr Attributes() : read(true), write(true), exec(false), user(true), _unused(0) {}
		} attr;

		uint8_t* data = nullptr;

		Page() = default;
		~Page() = default;

		bool is_cow() const noexcept { return false; }
		void make_cow() noexcept {}
	};

} // namespace loongarch
