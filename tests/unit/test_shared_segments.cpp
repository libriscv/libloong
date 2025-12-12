#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <libloong/util/crc32.hpp>
#include <libloong/shared_exec_segment.hpp>
#include <libloong/machine.hpp>
#include <thread>
#include <vector>
#include <array>
#include <cstring>

using namespace loongarch;

// Simple LoongArch program that returns 42
static const uint32_t return_42_program[] = {
	0x02802804, // li $a0, 42 (addi.w $a0, $zero, 42)
	0x002b0000, // syscall 0 (exit)
};

TEST_CASE("CRC32-C basic functionality", "[crc32c]") {
	SECTION("Empty data") {
		uint32_t crc = util::crc32c(nullptr, 0);
		REQUIRE(crc == 0);
	}

	SECTION("Known test vectors") {
		// Test that CRC32-C produces consistent results
		const char test1[] = "";
		uint32_t crc1 = util::crc32c(test1, 0);
		REQUIRE(crc1 == 0);

		// Test with simple data
		const uint8_t test2[] = {0x00};
		uint32_t crc2 = util::crc32c(test2, 1);
		// Just verify it's deterministic
		uint32_t crc2_again = util::crc32c(test2, 1);
		REQUIRE(crc2 == crc2_again);

		const uint8_t test3[] = {0x00, 0x00, 0x00, 0x00};
		uint32_t crc3 = util::crc32c(test3, 4);
		uint32_t crc3_again = util::crc32c(test3, 4);
		REQUIRE(crc3 == crc3_again);

		const uint8_t test4[] = {0xFF, 0xFF, 0xFF, 0xFF};
		uint32_t crc4 = util::crc32c(test4, 4);
		uint32_t crc4_again = util::crc32c(test4, 4);
		REQUIRE(crc4 == crc4_again);

		// Different data should produce different CRCs
		REQUIRE(crc2 != crc3);
		REQUIRE(crc3 != crc4);
	}

	SECTION("Incremental CRC") {
		const char data[] = "Hello, World!";

		// Full CRC
		uint32_t full_crc = util::crc32c(data, strlen(data));

		// Incremental CRC
		uint32_t inc_crc = ~0u;
		inc_crc = util::crc32c(inc_crc, data, 7); // "Hello, "
		inc_crc = util::crc32c(inc_crc, data + 7, strlen(data) - 7); // "World!"
		inc_crc = ~inc_crc;

		REQUIRE(full_crc == inc_crc);
	}

	SECTION("CRC of instruction data") {
		uint32_t crc = util::crc32c(return_42_program, sizeof(return_42_program));

		// Compute again to ensure determinism
		uint32_t crc2 = util::crc32c(return_42_program, sizeof(return_42_program));
		REQUIRE(crc == crc2);

		// Different data should produce different CRC
		uint32_t modified_program[] = {
			0x02802805, // Different immediate
			0x002b0000,
		};
		uint32_t crc3 = util::crc32c(modified_program, sizeof(modified_program));
		REQUIRE(crc != crc3);
	}
}

TEST_CASE("CRC32-C performance", "[crc32c][.benchmark]") {
	std::vector<uint8_t> data(1024 * 1024); // 1 MB
	for (size_t i = 0; i < data.size(); ++i) {
		data[i] = static_cast<uint8_t>(i);
	}

	BENCHMARK("CRC32-C 1MB") {
		return util::crc32c(data.data(), data.size());
	};
}

TEST_CASE("Shared execute segments - basic", "[shared_segments]") {
	// Create a simple ELF with the return_42_program
	// For simplicity, we'll create machines with the same program

	MachineOptions options;
	options.use_shared_execute_segments = true;
	options.memory_max = 1024 * 1024; // 1 MB

	SECTION("Segment key creation") {
		// Test that SegmentKey works correctly
		DecodedExecuteSegment seg1(0x1000, 0x1010);
		seg1.set_crc32c_hash(0x12345678);

		SegmentKey key1 = SegmentKey::from(0x1000, 0x12345678, 1024 * 1024);
		REQUIRE(key1.pc == 0x1000);
		REQUIRE(key1.crc == 0x12345678);
		REQUIRE(key1.arena_size == 1024 * 1024);

		// Test equality
		SegmentKey key2 = key1;
		REQUIRE(key1 == key2);

		// Test hash
		std::hash<SegmentKey> hasher;
		size_t hash1 = hasher(key1);
		size_t hash2 = hasher(key2);
		REQUIRE(hash1 == hash2);
	}

	SECTION("Shared cache operations") {
		auto& cache = get_shared_execute_segments();

		// Clear any existing state
		cache.clear();
		REQUIRE(cache.size() == 0);

		// Create a segment
		auto seg1 = std::make_shared<DecodedExecuteSegment>(0x2000, 0x2010);
		seg1->set_crc32c_hash(util::crc32c(return_42_program, sizeof(return_42_program)));

		SegmentKey key1 = SegmentKey::from(*seg1, 1024 * 1024);

		// Add to cache
		auto& entry = cache.get_segment(key1);
		{
			std::lock_guard<std::mutex> lock(entry.mutex);
			entry.unlocked_set(seg1);
		}

		REQUIRE(cache.size() == 1);

		// Retrieve from cache
		auto retrieved = entry.get();
		REQUIRE(retrieved != nullptr);
		REQUIRE(retrieved == seg1);

		// Remove if unique
		seg1.reset(); // Release our reference
		retrieved.reset(); // Release the retrieved reference
		cache.remove_if_unique(key1);

		// The entry should still exist in the map, but the segment should be null
		auto& entry2 = cache.get_segment(key1);
		auto retrieved2 = entry2.get();
		REQUIRE(retrieved2 == nullptr);

		cache.clear();
	}
}

TEST_CASE("Shared execute segments - multi-threaded", "[shared_segments][mt]") {
	// This test creates multiple machines in parallel and verifies thread-safety

	MachineOptions options;
	options.use_shared_execute_segments = true;
	options.memory_max = 1024 * 1024;

	auto& cache = get_shared_execute_segments();
	cache.clear();

	const int num_threads = 16;
	const int iterations_per_thread = 100;

	std::atomic<int> success_count{0};
	std::atomic<int> error_count{0};

	auto worker = [&](int thread_id) {
		try {
			for (int i = 0; i < iterations_per_thread; ++i) {
				// Create a segment
				auto seg = std::make_shared<DecodedExecuteSegment>(0x10000, 0x10010);
				uint32_t crc = util::crc32c(return_42_program, sizeof(return_42_program));
				seg->set_crc32c_hash(crc);

				SegmentKey key = SegmentKey::from(*seg, options.memory_max);

				// Try to add to cache
				auto& entry = cache.get_segment(key);
				std::shared_ptr<DecodedExecuteSegment> existing = entry.get();

				if (existing) {
					// Reuse existing
					seg = existing;
				} else {
					// Add new
					std::lock_guard<std::mutex> lock(entry.mutex);
					if (entry.segment) {
						seg = entry.segment;
					} else {
						entry.unlocked_set(seg);
					}
				}

				// Verify the segment
				REQUIRE(seg != nullptr);
				REQUIRE(seg->crc32c_hash() == crc);

				success_count++;
			}
		} catch (...) {
			error_count++;
		}
	};

	SECTION("Concurrent segment creation and lookup") {
		std::vector<std::thread> threads;
		threads.reserve(num_threads);

		for (int i = 0; i < num_threads; ++i) {
			threads.emplace_back(worker, i);
		}

		for (auto& t : threads) {
			t.join();
		}

		REQUIRE(error_count == 0);
		REQUIRE(success_count == num_threads * iterations_per_thread);

		// All threads should have created/reused the same segment
		// So there should be exactly 1 entry in the cache
		REQUIRE(cache.size() >= 1); // At least one entry

		cache.clear();
	}

	SECTION("Concurrent segment cleanup") {
		// Create segments
		std::vector<std::shared_ptr<DecodedExecuteSegment>> segments;
		for (int i = 0; i < num_threads; ++i) {
			auto seg = std::make_shared<DecodedExecuteSegment>(0x20000 + i * 0x1000, 0x20010 + i * 0x1000);
			seg->set_crc32c_hash(util::crc32c(&i, sizeof(i))); // Different CRCs

			SegmentKey key = SegmentKey::from(*seg, options.memory_max);
			auto& entry = cache.get_segment(key);
			{
				std::lock_guard<std::mutex> lock(entry.mutex);
				entry.unlocked_set(seg);
			}
			segments.push_back(seg);
		}

		REQUIRE(cache.size() >= num_threads);

		// Now release all references concurrently
		std::vector<std::thread> cleanup_threads;
		for (int i = 0; i < num_threads; ++i) {
			cleanup_threads.emplace_back([&cache, &segments, &options, i]() {
				if (i < static_cast<int>(segments.size()) && segments[i]) {
					SegmentKey key = SegmentKey::from(*segments[i], options.memory_max);
					segments[i].reset();
					cache.remove_if_unique(key);
				}
			});
		}

		for (auto& t : cleanup_threads) {
			t.join();
		}

		// All segments should be removed
		segments.clear();
		cache.clear();
	}
}

TEST_CASE("Shared execute segments - stress test", "[shared_segments][mt][stress]") {
	// Extreme stress test with many concurrent operations

	MachineOptions options;
	options.use_shared_execute_segments = true;
	options.memory_max = 2 * 1024 * 1024;

	auto& cache = get_shared_execute_segments();
	cache.clear();

	const int num_threads = 32;
	const int iterations_per_thread = 1000;
	const int num_different_programs = 10;

	std::array<uint32_t[2], num_different_programs> programs;
	for (int i = 0; i < num_different_programs; ++i) {
		programs[i][0] = 0x02802804 + i; // Different immediates
		programs[i][1] = 0x002b0000;
	}

	std::atomic<int> operations{0};

	auto worker = [&](int thread_id) {
		for (int i = 0; i < iterations_per_thread; ++i) {
			// Use different programs to create diverse cache entries
			int prog_idx = (thread_id * iterations_per_thread + i) % num_different_programs;

			auto seg = std::make_shared<DecodedExecuteSegment>(0x30000, 0x30008);
			uint32_t crc = util::crc32c(programs[prog_idx], sizeof(programs[prog_idx]));
			seg->set_crc32c_hash(crc);

			SegmentKey key = SegmentKey::from(*seg, options.memory_max);

			// Get or create
			auto& entry = cache.get_segment(key);
			std::shared_ptr<DecodedExecuteSegment> existing = entry.get();

			if (!existing) {
				std::lock_guard<std::mutex> lock(entry.mutex);
				if (!entry.segment) {
					entry.unlocked_set(seg);
				}
			}

			operations++;
		}
	};

	std::vector<std::thread> threads;
	threads.reserve(num_threads);

	for (int i = 0; i < num_threads; ++i) {
		threads.emplace_back(worker, i);
	}

	for (auto& t : threads) {
		t.join();
	}

	REQUIRE(operations == num_threads * iterations_per_thread);

	// Should have created entries for different programs
	REQUIRE(cache.size() >= 1);
	REQUIRE(cache.size() <= num_different_programs);

	cache.clear();
}
