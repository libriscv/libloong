#include "crc32.hpp"

// Include x86 intrinsics before entering namespace to avoid conflicts
#if defined(__x86_64__) || defined(_M_X64)
#include <nmmintrin.h>
#elif defined(__aarch64__) || defined(_M_ARM64)
#include <arm_acle.h>
#endif

namespace loongarch {
namespace util {

// Check if pointer is aligned
static inline bool is_aligned(const uint8_t* buffer, int align) noexcept {
	return (reinterpret_cast<uintptr_t>(buffer) & (align - 1)) == 0;
}

// CRC32-C polynomial (Castagnoli): 0x82F63B78
constexpr uint32_t CRC32C_POLYNOMIAL = 0x82F63B78;

// Software fallback using table-based CRC32-C
static uint32_t crc32c_sw(uint32_t crc, const void* vdata, size_t len)
{
	return crc32_sw<CRC32C_POLYNOMIAL>(crc, vdata, len);
}

// Hardware-accelerated implementations
#if defined(__x86_64__) || defined(_M_X64)
// x86-64 SSE4.2 implementation

__attribute__((target("sse4.2")))
static uint32_t crc32c_hw_x86(uint32_t crc, const uint8_t* buf, size_t len)
{
	// Align to 8-byte boundary
	while (len && (reinterpret_cast<uintptr_t>(buf) & 7)) {
		crc = _mm_crc32_u8(crc, *buf++);
		--len;
	}

	// Process 8 bytes at a time
	while (len >= 8) {
		crc = _mm_crc32_u64(crc, *reinterpret_cast<const uint64_t*>(buf));
		buf += 8;
		len -= 8;
	}

	// Process remaining bytes
	while (len--) {
		crc = _mm_crc32_u8(crc, *buf++);
	}

	return crc;
}

// Runtime check for SSE4.2 support
static bool has_sse42()
{
	static const bool supported = __builtin_cpu_supports("sse4.2");
	return supported;
}

#elif defined(__aarch64__) || defined(_M_ARM64)
// ARM64 CRC32 implementation

static uint32_t crc32c_hw_arm(uint32_t crc, const uint8_t* buf, size_t len)
{
	// Align to 8-byte boundary
	while (len && (reinterpret_cast<uintptr_t>(buf) & 7)) {
		crc = __crc32cb(crc, *buf++);
		--len;
	}

	// Process 8 bytes at a time
	while (len >= 8) {
		crc = __crc32cd(crc, *reinterpret_cast<const uint64_t*>(buf));
		buf += 8;
		len -= 8;
	}

	// Process remaining bytes
	while (len--) {
		crc = __crc32cb(crc, *buf++);
	}

	return crc;
}

#elif defined(__loongarch64)
// LoongArch64 CRC32 implementation
// LoongArch has dedicated CRC32 instructions: crc.w.b.w, crc.w.h.w, crc.w.w.w, crc.w.d.w

static inline uint32_t __loongarch_crc32c_b(uint32_t crc, uint8_t val)
{
	uint32_t result;
	asm volatile("crc.w.b.w %0, %1, %2" : "=r"(result) : "r"(val), "r"(crc));
	return result;
}

static inline uint32_t __loongarch_crc32c_h(uint32_t crc, uint16_t val)
{
	uint32_t result;
	asm volatile("crc.w.h.w %0, %1, %2" : "=r"(result) : "r"(val), "r"(crc));
	return result;
}

static inline uint32_t __loongarch_crc32c_w(uint32_t crc, uint32_t val)
{
	uint32_t result;
	asm volatile("crc.w.w.w %0, %1, %2" : "=r"(result) : "r"(val), "r"(crc));
	return result;
}

static inline uint32_t __loongarch_crc32c_d(uint32_t crc, uint64_t val)
{
	uint32_t result;
	asm volatile("crc.w.d.w %0, %1, %2" : "=r"(result) : "r"(val), "r"(crc));
	return result;
}

static uint32_t crc32c_hw_loongarch(uint32_t crc, const uint8_t* buf, size_t len)
{
	// Align to 8-byte boundary
	while (len && (reinterpret_cast<uintptr_t>(buf) & 7)) {
		crc = __loongarch_crc32c_b(crc, *buf++);
		--len;
	}

	// Process 8 bytes at a time
	while (len >= 8) {
		crc = __loongarch_crc32c_d(crc, *reinterpret_cast<const uint64_t*>(buf));
		buf += 8;
		len -= 8;
	}

	// Process 4 bytes
	if (len >= 4) {
		crc = __loongarch_crc32c_w(crc, *reinterpret_cast<const uint32_t*>(buf));
		buf += 4;
		len -= 4;
	}

	// Process 2 bytes
	if (len >= 2) {
		crc = __loongarch_crc32c_h(crc, *reinterpret_cast<const uint16_t*>(buf));
		buf += 2;
		len -= 2;
	}

	// Process remaining byte
	if (len) {
		crc = __loongarch_crc32c_b(crc, *buf);
	}

	return crc;
}

#endif

// Public interface
uint32_t crc32c(uint32_t crc, const void* data, size_t len)
{
	const uint8_t* buf = static_cast<const uint8_t*>(data);

#if defined(__x86_64__) || defined(_M_X64)
	if (has_sse42()) {
		return crc32c_hw_x86(crc, buf, len);
	}
#elif defined(__aarch64__) || defined(_M_ARM64)
	return crc32c_hw_arm(crc, buf, len);
#elif defined(__loongarch64)
	return crc32c_hw_loongarch(crc, buf, len);
#endif

	// Software fallback
	return crc32c_sw(crc, data, len);
}

uint32_t crc32c(const void* data, size_t len)
{
	return ~crc32c(0xFFFFFFFF, data, len);
}

} // namespace util
} // namespace loongarch
