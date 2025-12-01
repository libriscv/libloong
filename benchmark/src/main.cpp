#include "benchmark.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>

// Forward declarations
namespace benchmark {
	void initialize(const std::string& binary_path);
	void run_all_benchmarks(int samples);
}

int main(int argc, char* argv[]) {
	// Default configuration
	int samples = 200;
	std::string binary_path = GUEST_BINARY_PATH;

	// Parse command line arguments
	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];

		if (arg == "--help" || arg == "-h") {
			printf("Usage: %s [options]\n", argv[0]);
			printf("\nOptions:\n");
			printf("  --samples N, -s N    Number of samples to run (default: 200)\n");
			printf("  --binary PATH, -b PATH  Path to guest binary (default: built-in)\n");
			printf("  --help, -h           Show this help message\n");
			printf("\nDescription:\n");
			printf("  Benchmarks libloong vmcall overhead with various argument counts.\n");
			printf("  Each benchmark runs multiple samples with 1000 iterations each.\n");
			printf("  Results show median, lowest, highest, and percentile timings.\n");
			return 0;
		}
		else if ((arg == "--samples" || arg == "-s") && i + 1 < argc) {
			samples = std::atoi(argv[++i]);
			if (samples <= 0) {
				fprintf(stderr, "Error: samples must be positive\n");
				return 1;
			}
		}
		else if ((arg == "--binary" || arg == "-b") && i + 1 < argc) {
			binary_path = argv[++i];
		}
		else {
			fprintf(stderr, "Error: unknown argument '%s'\n", arg.c_str());
			fprintf(stderr, "Use --help for usage information\n");
			return 1;
		}
	}

	try {
		// Initialize the benchmark environment
		printf("Initializing libloong benchmark...\n");
		printf("Guest binary: %s\n", binary_path.c_str());
		printf("\n");

		benchmark::initialize(binary_path);

		// Run all benchmarks
		benchmark::run_all_benchmarks(samples);

		return 0;
	}
	catch (const std::exception& e) {
		fprintf(stderr, "Error: %s\n", e.what());
		return 1;
	}
}
