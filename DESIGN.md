# Vitte Benchmark Suite — Design Document

## Overview

The Vitte Benchmark Suite (`vitte-bench`) is a minimal, portable C11 benchmarking harness designed to measure performance of critical Vitte compiler and runtime components.

## Design Goals

1. **Simplicity**: Minimal dependencies, single-file builds, easy to extend
2. **Portability**: Cross-platform (Windows, macOS, Linux, Unix)
3. **Accuracy**: Robust statistical sampling, monotonic timing
4. **Extensibility**: Simple registry pattern for adding benchmarks
5. **Performance**: Zero-overhead abstraction, no allocations during measurement

## Architecture

### Layers

```
┌─────────────────────────────────┐
│   benchc (main executable)      │  Command-line interface
├─────────────────────────────────┤
│   runner.c                      │  Orchestration
├─────────────────────────────────┤
│   Individual benchmarks         │  bm_*.c
│   (micro & macro)               │
├─────────────────────────────────┤
│   Core library (benchlib)       │
│   - bench_time.c                │  Monotonic timing
│   - bench_stats.c               │  Statistics
│   - bench_registry.c            │  Case registry
└─────────────────────────────────┘
```

### Core Components

#### 1. Timing (`bench_time.c`)

- **Platform abstraction** for monotonic clocks
- **macOS**: `mach_absolute_time()` with timebase conversion
- **POSIX**: `clock_gettime(CLOCK_MONOTONIC)` with nanosecond precision
- **Windows**: `QueryPerformanceCounter()` with frequency conversion

#### 2. Registry (`bench_registry.c`)

- **Static array** of `bench_case` structures
- **No allocation**: Registry is compile-time constant
- **Extensible**: New benchmarks added via forward declarations and array entries
- **Lookup**: Linear search (acceptable for small benchmark counts)

#### 3. Statistics (`bench_stats.c`)

- **Robust mean** via Kahan summation (reduces floating-point error)
- **Percentiles** via sorting (p50, p95, p99)
- **Ignores outliers**: Handles NaN/Inf gracefully
- **Standard deviation**: For variance analysis

#### 4. Runner (`runner.c`, `bench_main.c`)

- **Configuration-driven**: `bench_runner_config_t` controls parameters
- **Micro benchmarks**: Fixed iterations per sample
- **Macro benchmarks**: Time-bounded execution (target duration)
- **Warmup**: Preliminary calls to stabilize caches/branch predictors

## Benchmark Types

### Micro Benchmarks

Measure **single operations** in nanoseconds.

**Characteristics:**
- Run fixed iterations (default: 1,000,000)
- Measure time per operation
- Sensitive to cache behavior, branch prediction
- Results: ns/op

**Examples:**
- `micro:add` — integer addition
- `micro:hash` — FNV-1a 64-bit hash
- `micro:memcpy` — memory copy

### Macro Benchmarks

Measure **workloads** at millisecond to second scales.

**Characteristics:**
- Run for target duration (default: 2 seconds per sample)
- Amortize overhead over larger payloads
- Representative of real-world usage
- Results: operations/second or ms/op

**Examples:**
- `macro:json_parse` — JSON parsing throughput

## Adding Benchmarks

### Step 1: Create implementation file

```c
// src/micro/bm_myop.c
#include "bench/bench.h"

void bm_myop(void* ctx) {
  // Benchmark body: run operation N times
  // Use volatile to prevent optimization
  // Use external "sink" variable to prevent DCE
}
```

### Step 2: Register in `bench_registry.c`

```c
extern void bm_myop(void* ctx);

static const bench_case registry[] = {
  // ... existing cases ...
  {
    .id = "micro:myop",
    .kind = BENCH_MICRO,
    .fn = bm_myop,
    .ctx = NULL,
  },
};
```

### Step 3: Update CMakeLists.txt

```cmake
add_executable(benchc
  src/bench/bench_main.c
  src/micro/bm_myop.c  # Add here
  # ...
)
```

## Platform Support

| Platform | Compiler | Arch | Status |
|----------|----------|------|--------|
| Linux    | GCC, Clang | x64 | ✓ Supported |
| macOS    | Clang | x64, ARM64 | ✓ Supported |
| Windows  | MSVC | x64 | ✓ Supported |
| FreeBSD  | Clang | x64 | ✓ Supported |

## Performance Considerations

### Avoiding Optimization

1. **Volatile variables**: Prevent dead code elimination
2. **Opaque pointers**: Use external variables as sinks
3. **Inline asm barriers**: Prevent instruction reordering
4. **Warmup iterations**: Stabilize CPU state

### Time Granularity

- **Linux/macOS**: nanosecond precision (`CLOCK_MONOTONIC`)
- **Windows**: microsecond precision (QueryPerformanceCounter)
- **Fallback**: Degrades to microsecond if finer unavailable

## Statistical Robustness

### Kahan Summation (Mean)

Reduces floating-point rounding errors in sum calculation:

```c
double sum = 0.0, c = 0.0;
for (int i = 0; i < n; i++) {
  double y = samples[i] - c;
  double t = sum + y;
  c = (t - sum) - y;
  sum = t;
}
double mean = sum / n;
```

### Percentile Calculation

- Sort samples
- p50 = samples[n/2]
- p95 = samples[(int)(n*0.95)]
- p99 = samples[(int)(n*0.99)]

## Output Formats

### Text (stdout)

```
Benchmark              Mean         Median       p95          Min          Max
micro:add              12.34 ns     12.10 ns     13.20 ns     11.90 ns     14.50 ns
macro:json_parse       1.23 ms      1.20 ms      1.45 ms      1.10 ms      1.80 ms
```

### CSV

Comma-separated values for spreadsheet/plotting tools:

```
benchmark_id,kind,mean_ns,median_ns,p95_ns,p99_ns,min_ns,max_ns,stddev_ns,samples,duration_s
micro:add,micro,12.34,12.10,13.20,11.90,14.50,0.45,7,0.0000
```

## Thread Safety

**Note:** Current implementation is **NOT thread-safe**. Each benchmark runs sequentially.

For multi-threaded benchmarks in the future:
- Per-thread result buffers
- Atomic operations for registry
- Lock-free statistics aggregation

## Future Extensions

1. **JSON output**: Structured results for CI/CD pipelines
2. **Histogram output**: Distribution visualization
3. **Comparative mode**: A/B benchmarking (baseline vs. optimized)
4. **Regression detection**: Automatic threshold warnings
5. **Parallelism**: Run multiple benchmarks concurrently
6. **Custom units**: Bytes/second, instructions/cycle, etc.

## References

- Google Benchmark: https://github.com/google/benchmark
- Criterion: https://github.com/Snaipe/Criterion
- Hyperfine: https://github.com/sharkdp/hyperfine
