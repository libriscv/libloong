#include <string>

namespace loongarch {
	extern const std::string bintr_code =
		R"EOF(
#if (defined(__TINYC__) && defined(__FreeBSD__))
#define int8_t   char
#define uint8_t  unsigned char
#define int16_t  short
#define uint16_t unsigned short
#define int32_t  int
#define uint32_t unsigned int
#define int64_t  long long
#define uint64_t unsigned long long
#define uintptr_t unsigned long long
#elif defined(__TINYC__) && defined(_WIN32)
#define int8_t   char
#define uint8_t  unsigned char
#define int16_t  short
#define uint16_t unsigned short
#define int32_t  int
#define uint32_t unsigned int
#define int64_t  long long
#define uint64_t unsigned long long
#define uintptr_t unsigned long long
#elif defined(__TINYC__)
#define int8_t   signed char
#define uint8_t  unsigned char
#define int16_t  short
#define uint16_t unsigned short
#define int32_t  int
#define uint32_t unsigned int
#define int64_t  long
#define uint64_t unsigned long
#define uintptr_t unsigned long
#else
#include <stdint.h>
#endif

#ifndef INT64_MIN
#define INT64_MIN  0x8000000000000000LL
#endif
#ifndef INT32_MIN
#define INT32_MIN  0x80000000L
#endif

#define LIKELY(x) __builtin_expect((x), 1)
#define UNLIKELY(x) __builtin_expect((x), 0)
#define ILLEGAL_OPCODE  0
#define MISALIGNED_INSTRUCTION 4
#define VISIBLE  __attribute__((visibility("default")))
#define INTERNAL __attribute__((visibility("hidden")))

typedef uint64_t addr_t;
typedef int64_t saddr_t;
#define XLEN  64

// LoongArch instructions are always 4-byte aligned
#define LA_ALIGN_MASK 0x3

#ifdef __TINYC__
#define UNREACHABLE() /**/
static inline float fminf(float x, float y) {
	return (x < y) ? x : y;
}
static inline double fmin(double x, double y) {
	return (x < y) ? x : y;
}
static inline float fmaxf(float x, float y) {
	return (x >= y) ? x : y;
}
static inline double fmax(double x, double y) {
	return (x >= y) ? x : y;
}
static inline uint32_t do_bswap32(uint32_t x) {
	return (x << 24 | (x & 0xFF00) << 8 | (x & 0xFF0000) >> 8 | x >> 24);
}
#define do_bswap64(x) (do_bswap32((x) >> 32) | ((uint64_t)do_bswap32(x) << 32))
#define do_clz(x) api.clz(x)
#define do_clzl(x) api.clzl(x)
#define do_ctz(x) api.ctz(x)
#define do_ctzl(x) api.ctzl(x)
#define do_cpop(x) api.cpop(x)
#define do_cpopl(x) api.cpopl(x)
#else
#define UNREACHABLE() __builtin_unreachable()
#define do_bswap32(x) __builtin_bswap32(x)
#define do_bswap64(x) __builtin_bswap64(x)
#define do_clz(x) __builtin_clz(x)
#define do_clzl(x) __builtin_clzl(x)
#define do_ctz(x) __builtin_ctz(x)
#define do_ctzl(x) __builtin_ctzl(x)
#define do_cpop(x) __builtin_popcount(x)
#define do_cpopl(x) __builtin_popcountl(x)
#define fminf(x, y) __builtin_fminf(x, y)
#define fmin(x, y) __builtin_fmin(x, y)
#define fmaxf(x, y) __builtin_fmaxf(x, y)
#define fmax(x, y) __builtin_fmax(x, y)
#endif

#ifdef __HAVE_BUILTIN_SPECULATION_SAFE_VALUE
#define SPECSAFE(x) __builtin_speculation_safe_value(x)
#else
#define SPECSAFE(x) (x)
#endif

// Floating-point register union
typedef union {
	int8_t   b[32];
	int16_t  h[16];
	int32_t  w[8];
	int64_t  d[4];
	uint8_t  bu[32];
	uint16_t hu[16];
	uint32_t wu[8];
	uint64_t du[4];
	float    f[8];
	double   df[4];
} lasx_reg;

// CPU structure - simplified version for binary translation
__attribute__((aligned(LA_MACHINE_ALIGNMENT)))
typedef struct {
	addr_t  pc;        // Program counter
	addr_t  r[32];     // General-purpose registers
	addr_t padding[3]; // Padding to align vectors
	lasx_reg vr[32];   // Vector/FP registers
	uint32_t fcsr;     // Floating-point control/status register
	uint8_t  fcc;      // Floating-point condition codes
} CPU;

typedef void (*syscall_t) (CPU*);
typedef void (*handler_t) (CPU*, uint32_t);

// Callback table for interfacing with the emulator
// MUST match the structure in tr_compiler.cpp
static struct CallbackTable {
	syscall_t* syscalls;
	void (*unknown_syscall)(CPU*, addr_t);
	handler_t* handlers;
	int  (*syscall)(CPU*, uint64_t, uint64_t, addr_t);
	void (*exception) (CPU*, addr_t, int);
	void (*trace) (CPU*, const char*, addr_t, uint32_t);
	void (*log) (CPU*, addr_t, const char*);
	float  (*sqrtf32)(float);
	double (*sqrtf64)(double);
	int (*clz) (uint32_t);
	int (*clzl) (uint64_t);
	int (*ctz) (uint32_t);
	int (*ctzl) (uint64_t);
	int (*cpop) (uint32_t);
	int (*cpopl) (uint64_t);
} api;

// Memory arena access
INTERNAL static int32_t arena_offset;
#define ARENA_AT(cpu, x)  (*(uint8_t **)((uintptr_t)cpu + arena_offset) + (x))

// Instruction counter access
INTERNAL static int32_t ic_offset;
#define INS_COUNTER(cpu) (*(uint64_t *)((uintptr_t)cpu + ic_offset))
#define MAX_COUNTER(cpu) (*(uint64_t *)((uintptr_t)cpu + ic_offset + 8))

// Flat memory arena - libloong always uses flat arena
#define rd8(cpu, addr) \
	*(uint8_t*)ARENA_AT(cpu, addr)
#define rd16(cpu, addr) \
	*(uint16_t*)ARENA_AT(cpu, addr)
#define rd32(cpu, addr) \
	*(uint32_t*)ARENA_AT(cpu, addr)
#define rd64(cpu, addr) \
	*(uint64_t*)ARENA_AT(cpu, addr)

#define wr8(cpu, addr, value) \
	*(uint8_t*)ARENA_AT(cpu, addr) = (value);
#define wr16(cpu, addr, value) \
	*(uint16_t*)ARENA_AT(cpu, addr) = (value);
#define wr32(cpu, addr, value) \
	*(uint32_t*)ARENA_AT(cpu, addr) = (value);
#define wr64(cpu, addr, value) \
	*(uint64_t*)ARENA_AT(cpu, addr) = (value);

static inline int do_syscall(CPU* cpu, uint64_t counter, uint64_t max_counter, addr_t sysno)
{
	INS_COUNTER(cpu) = counter; // Reveal instruction counters
	MAX_COUNTER(cpu) = max_counter;
	addr_t old_pc = cpu->pc;
	if (LIKELY(sysno < LA_SYSCALLS_MAX)) {
		api.syscalls[SPECSAFE(sysno)](cpu);
	} else {
		api.unknown_syscall(cpu, sysno);
	}
	// Resume if the system call did not modify PC, or hit a limit
	return (cpu->pc != old_pc || counter >= MAX_COUNTER(cpu));
}

#define JUMP_TO(addr) \
	pc = addr & ~(addr_t)LA_ALIGN_MASK;

// https://stackoverflow.com/questions/28868367/getting-the-high-part-of-64-bit-integer-multiplication
// As written by catid
static inline uint64_t MUL128(
	uint64_t* r_hi,
	const uint64_t x,
	const uint64_t y)
{
	const uint64_t x0 = (uint32_t)x, x1 = x >> 32;
	const uint64_t y0 = (uint32_t)y, y1 = y >> 32;
	const uint64_t p11 = x1 * y1, p01 = x0 * y1;
	const uint64_t p10 = x1 * y0, p00 = x0 * y0;

	// 64-bit product + two 32-bit values
	const uint64_t middle = p10 + (p00 >> 32) + (uint32_t)p01;

	// 64-bit product + two 32-bit values
	*r_hi = p11 + (middle >> 32) + (p01 >> 32);

	// Add LOW PART and lower half of MIDDLE PART
	return (middle << 32) | (uint32_t)p00;
}

#ifdef EMBEDDABLE_CODE
static
#else
extern VISIBLE
#endif
void init(struct CallbackTable* table, int32_t arena_off, int32_t ins_counter_off)
{
	api = *table;
	arena_offset = arena_off;
	ic_offset = ins_counter_off;
}

typedef struct {
	uint64_t ic;
	uint64_t max_ic;
} ReturnValues;
)EOF";
}
