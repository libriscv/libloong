#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace loongarch {
namespace util {

// Compile-time CRC32 table generation
template <uint32_t POLYNOMIAL>
inline constexpr auto gen_crc32_table()
{
	constexpr auto num_iterations = 8;
	auto crc32_table = std::array<uint32_t, 256> {};

	for (auto byte = 0u; byte < crc32_table.size(); ++byte) {
		auto crc = byte;

		for (auto i = 0; i < num_iterations; ++i) {
			auto mask = -(crc & 1);
			crc = (crc >> 1) ^ (POLYNOMIAL & mask);
		}

		crc32_table[byte] = crc;
	}
	return crc32_table;
}

// Software fallback CRC32 with custom polynomial
template <uint32_t POLYNOMIAL = 0xEDB88320>
inline constexpr uint32_t crc32_sw(uint32_t crc, const void* vdata, size_t len)
{
	constexpr auto crc32_table = gen_crc32_table<POLYNOMIAL>();

	auto* data = static_cast<const uint8_t*>(vdata);
	for (size_t i = 0; i < len; ++i) {
		crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
	}
	return crc;
}

template <uint32_t POLYNOMIAL = 0xEDB88320>
inline constexpr uint32_t crc32(const void* vdata, size_t len)
{
	return ~crc32_sw<POLYNOMIAL>(0xFFFFFFFF, vdata, len);
}

// CRC32-C with hardware acceleration
// Uses Castagnoli polynomial: 0x82F63B78 (different from standard CRC32)
// This is the variant with hardware support on x86 (SSE4.2), ARM (CRC32), and LoongArch
uint32_t crc32c(const void* data, size_t len);
uint32_t crc32c(uint32_t partial, const void* data, size_t len);

} // namespace util
} // namespace loongarch
