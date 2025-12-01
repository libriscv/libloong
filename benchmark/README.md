# libloong Benchmark Suite

A focused benchmarking suite for measuring libloong's vmcall overhead and performance characteristics.

## Overview

This benchmark suite measures the performance of calling guest functions from the host via `vmcall`. It provides clean infrastructure for measuring:

- Pure vmcall overhead (empty function)
- Argument passing overhead (0-8 arguments)
- Statistical analysis (median, percentiles, min/max)

## Building

### Quick Start

From the benchmark directory:

```bash
./build.sh
```

### Manual Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

This will:
1. Build the LoongArch guest program (`benchmark_guest.elf`)
2. Build the host-side benchmark executable (`bench`)

## Running

```bash
./build/bench
```

Options:
- `--samples N` or `-s N`: Number of samples (default: 200)
- `--binary PATH` or `-b PATH`: Custom guest binary path
- `--help` or `-h`: Show help message

Example:
```bash
./build/bench --samples 500
```

## Architecture

### Guest Program ([guest/guest_main.cpp](guest/guest_main.cpp))

The guest program is compiled statically with `-Wl,-Ttext-segment=0x200000` to load inside libloong's flat memory arena.

### Host Infrastructure

- [include/benchmark.hpp](include/benchmark.hpp): Core benchmarking primitives
  - High-resolution timing
  - Statistical analysis (median, percentiles)
  - Memory barriers to prevent compiler reordering

- [src/benchmark.cpp](src/benchmark.cpp): Test implementations
  - Machine initialization
  - vmcall test functions
  - Result aggregation and reporting

- [src/main.cpp](src/main.cpp): Entry point
  - Command-line argument parsing
  - Benchmark orchestration

## Results Format

Each benchmark reports:
- **median**: 50th percentile (most representative)
- **lowest**: Best case timing
- **p75/p90/p99**: 75th, 90th, and 99th percentiles (tail latency indicators)

All timings are in nanoseconds with benchmark overhead subtracted.

### Example Results

On a modern x86_64 system (200 samples × 1000 iterations):

```
=== VMCall Overhead Tests ===
                  empty function	median:      6ns	lowest:      6ns	[p75:      6ns  p90:      6ns  p99:      9ns]

=== Argument Passing Overhead ===
                          args=0	median:      7ns	lowest:      7ns	[p75:      7ns  p90:      7ns  p99:      9ns]
                          args=1	median:      7ns	lowest:      7ns	[p75:      7ns  p90:      7ns  p99:     10ns]
                          args=2	median:      7ns	lowest:      7ns	[p75:      7ns  p90:      7ns  p99:     12ns]
                          args=3	median:      9ns	lowest:      8ns	[p75:      9ns  p90:      9ns  p99:     13ns]
                          args=4	median:     10ns	lowest:      9ns	[p75:     10ns  p90:     10ns  p99:     13ns]
                          args=5	median:     11ns	lowest:     11ns	[p75:     11ns  p90:     11ns  p99:     13ns]
                          args=6	median:     12ns	lowest:     12ns	[p75:     12ns  p90:     12ns  p99:     14ns]
                          args=7	median:     13ns	lowest:     13ns	[p75:     13ns  p90:     13ns  p99:     17ns]
                          args=8	median:     15ns	lowest:     15ns	[p75:     15ns  p90:     15ns  p99:     17ns]
```

These results demonstrate libloong's **extremely low vmcall overhead**, with bare function calls taking just ~6ns and increasing by only ~0.5-1ns per additional argument. The tight distribution (low variance between median and p99) shows consistent, predictable performance.

## Design Principles

Unlike the libriscv benchmark suite which has grown complex over time, this suite prioritizes:

1. **Simplicity**: Minimal, focused tests
2. **Clarity**: Clean separation of concerns
3. **Maintainability**: Well-documented, easy to extend
4. **Performance**: Efficient measurement infrastructure
5. **Statistical rigor**: Multiple samples with percentile analysis

## Extending

To add new benchmarks:

1. Add guest function to `guest/guest_main.cpp`
2. Add test implementation to `src/benchmark.cpp`
3. Call the test from `run_all_benchmarks()`

The infrastructure handles timing, statistics, and reporting automatically.

## Notes

- Each test runs 1000 iterations per sample by default
- Results are sorted for accurate percentile calculation
- Warmup iterations prevent cold-start effects
- Memory barriers prevent compiler optimizations from skewing results
- Static function addresses are cached to avoid lookup overhead
