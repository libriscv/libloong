#pragma once
#include <chrono>
#include <vector>
#include <string>
#include <functional>
#include <algorithm>
#include <cstdint>

namespace benchmark {

using time_point = std::chrono::time_point<std::chrono::high_resolution_clock>;
using test_func = std::function<void()>;
using setup_func = std::function<void()>;

// Benchmark result for a single test
struct BenchmarkResult {
	std::string name;
	int64_t median_ns;
	int64_t lowest_ns;
	int64_t p75_ns;  // 75th percentile
	int64_t p90_ns;  // 90th percentile
	int64_t p99_ns;  // 99th percentile
	int samples;
};

// Get current time with high resolution
inline time_point time_now() {
	return std::chrono::high_resolution_clock::now();
}

// Calculate time difference in nanoseconds
inline int64_t time_diff_ns(time_point start, time_point end) {
	return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}

// Run a single iteration of the test
template <int Iterations = 1000>
inline int64_t perform_test(test_func func) {
	// Warmup
	func();

	// Memory barrier to prevent reordering
	asm volatile("" ::: "memory");

	auto start = time_now();

	asm volatile("" ::: "memory");

	for (int i = 0; i < Iterations; i++) {
		func();
	}

	asm volatile("" ::: "memory");

	auto end = time_now();

	asm volatile("" ::: "memory");

	return time_diff_ns(start, end);
}

// Run multiple samples and compute statistics
template <int Iterations = 1000>
inline BenchmarkResult run_benchmark(
	const char* name,
	int samples,
	setup_func setup,
	test_func test,
	int64_t overhead = 0)
{
	std::vector<int64_t> results;
	results.reserve(samples);

	for (int i = 0; i < samples; i++) {
		// Run setup before each sample
		if (setup) {
			setup();
		}

		// Measured iteration
		int64_t time = perform_test<Iterations>(test);
		results.push_back(time / Iterations);
	}

	// Sort results for percentile calculation
	std::sort(results.begin(), results.end());

	BenchmarkResult result;
	result.name = name;
	result.samples = samples;
	result.median_ns = results[results.size() / 2] - overhead;
	result.lowest_ns = results[0] - overhead;
	result.p75_ns = results[3 * results.size() / 4] - overhead;
	result.p90_ns = results[9 * results.size() / 10] - overhead;
	result.p99_ns = results[99 * results.size() / 100] - overhead;

	return result;
}

// Print benchmark result in a nice format
inline void print_result(const BenchmarkResult& result) {
	printf("%32s\tmedian: %6ldns\tlowest: %6ldns\t[p75: %6ldns  p90: %6ldns  p99: %6ldns]\n",
		result.name.c_str(),
		result.median_ns,
		result.lowest_ns,
		result.p75_ns,
		result.p90_ns,
		result.p99_ns);
}

// Measure benchmark overhead (empty function)
template <int Iterations = 1000>
inline int64_t measure_overhead(int samples) {
	std::vector<int64_t> results;
	results.reserve(samples);

	for (int i = 0; i < samples; i++) {
		auto start = time_now();

		for (int j = 0; j < Iterations; j++) {
			asm volatile("" ::: "memory");
		}

		auto end = time_now();

		results.push_back(time_diff_ns(start, end) / Iterations);
	}

	std::sort(results.begin(), results.end());
	return results[results.size() / 2];
}

} // namespace benchmark
