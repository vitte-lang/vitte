# Vitte Benchmark Suite

Comprehensive benchmarking framework for the Vitte compiler and runtime, featuring 23 micro and macro benchmarks with production-ready infrastructure.

## Structure

```
bench/
├── CMakeLists.txt              # Build configuration
├── src/
│   ├── bench/                  # Framework (16 .c + 16 .h files)
│   │   ├── benchmark_init.c    # Centralized benchmark registration
│   │   ├── bench_main.c        # CLI entry point
│   │   ├── bench_registry.c    # Static benchmark registry
│   │   ├── bench_stats.c       # Statistical analysis (Kahan sum, percentiles)
│   │   ├── bench_time.c        # Monotonic nanosecond clock
│   │   ├── runner.c            # Micro/macro orchestration
│   │   ├── options.c           # CLI argument parsing
│   │   ├── output.c            # Result formatting (text/CSV)
│   │   ├── timing.c            # Scoped timers
│   │   ├── sample.c            # Dynamic sample buffers
│   │   ├── json_parser.c       # Lightweight JSON parser
│   │   ├── alloc.c             # Memory tracking
│   │   ├── strutil.c           # String utilities
│   │   ├── format.c            # Numeric formatting
│   │   ├── csv.c               # CSV writer
│   │   ├── log.c               # 4-level logging
│   │   └── [headers]           # Public APIs
│   │
│   ├── micro/ (15 benchmarks)
│   │   ├── bm_add.c                 # Integer addition throughput
│   │   ├── bm_array_access.c        # Cache patterns (seq/stride/random)
│   │   ├── bm_bitops.c              # Bitwise operations
│   │   ├── bm_branch_prediction.c   # Branch prediction behavior
│   │   ├── bm_cache_line_effects.c  # Cache line & alignment effects
│   │   ├── bm_conditional_move.c    # Branch vs CMOV comparison
│   │   ├── bm_data_dependency.c     # ILP vs data dependencies
│   │   ├── bm_division.c            # Division/modulo latency
│   │   ├── bm_floating_point.c      # FP operations & chains
│   │   ├── bm_function_call.c       # Call overhead (inline/deep)
│   │   ├── bm_hash.c                # Hashing throughput
│   │   ├── bm_loop_unroll.c         # Loop unrolling impacts
│   │   ├── bm_memcpy.c              # Memory copy throughput
│   │   ├── bm_recursion.c           # Recursive call overhead
│   │   └── bm_string_search.c       # String matching patterns
│   │
│   └── macro/ (8 benchmarks)
│       ├── bm_cache_effects.c       # Cache hierarchy effects
│       ├── bm_compression.c         # RLE & pattern compression
│       ├── bm_file_io_sim.c         # I/O patterns (seq/random/buffered)
│       ├── bm_json_parse.c          # JSON parsing workload
│       ├── bm_math_compute.c        # Matrix & trig operations
│       ├── bm_regex_match.c         # Pattern matching (glob/email/URL)
│       ├── bm_sort_algorithms.c     # Sorting (bubble/insertion/quick)
│       └── bm_string_ops.c          # String operations (concat/search/case)
│
└── README.md                   # This file
```

## Hash Suite Integration

- `bench/src/micro/bm_hash.c` must be part of your build whenever you want the `bench_register_micro_hash()` legacy hook or the new `bm_hash_register()` API. The translation unit exports the registrar so `bench/src/bench/register_builtin.c` can discover the hash micro-benchmarks.
- Legacy runners that lazily allocate hash buffers can release them deterministically (for long-lived or embedded harnesses) by calling `bench_micro_hash_release_all()` instead of waiting for process exit.

## Features

### Micro-benchmarks (15)
Fixed-iteration benchmarks measuring instruction-level performance:
- **Arithmetic**: addition, bitwise, division
- **Memory**: array access patterns, cache effects, memcpy
- **Control**: branches, conditional moves
- **FP**: floating-point operations with ILP
- **Calls**: function call overhead, recursion
- **Optimization**: loop unrolling, dependencies

### Macro-benchmarks (8)
Time-bounded workloads measuring realistic performance:
- **Data processing**: JSON parsing, compression, sorting
- **Text**: regex patterns, string operations
- **Computation**: matrix operations, math functions
- **Memory**: I/O patterns, cache hierarchy effects

### Framework
- **Zero external dependencies** (JSON parser built-in)
- **Cross-platform**: macOS (mach_absolute_time), Linux (clock_gettime), Windows (QueryPerformanceCounter)
- **Robust statistics**: Kahan summation, percentiles (p50/p95/p99), outlier detection
- **Output formats**: Text (aligned columns), CSV, JSON
- **Logging**: 4-level DEBUG/INFO/WARN/ERROR with timestamps
- **Memory tracking**: Allocation statistics without overhead

## Building

### Requirements
- C11 compiler (GCC, Clang, MSVC)
- CMake 3.20+
- Math library (libm on Unix)

### Build Steps

```bash
# From Vitte root
cd bench
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make vittec-bench
```

### Output
Executable: `build/vittec-bench`

## Usage

### List Available Benchmarks
```bash
./vittec-bench --list
./vittec-bench --list-full
```

### Run Specific Benchmarks
```bash
# Single micro-benchmark
./vittec-bench micro:add

# Multiple benchmarks
./vittec-bench micro:add micro:hash macro:json_small

# All benchmarks
./vittec-bench --all

# With filtering
./vittec-bench --all --filter hash
```

### Customize Execution
```bash
# Micro: 2M iterations, 10 samples
./vittec-bench --iters 2000000 --samples 10 micro:add

# Macro: 3 seconds per sample
./vittec-bench --seconds 3 macro:json_parse

# Reduce warmup
./vittec-bench --warmup 100 micro:add

# Export to CSV
./vittec-bench --csv results.csv --all
```

### Command-line Options
```
--help, -h              Show help
--list                  List benchmark case ids
--list-full             List ids with kind (micro/macro)
--all                   Run all registered benchmarks
--filter SUBSTR         Only include cases containing SUBSTR
--iters N               Micro: iterations per sample (default: 1000000)
--samples N             Number of samples (default: 7)
--seconds S             Macro: seconds per sample (default: 2.0)
--warmup N              Warmup calls before sampling (default: 1000)
--timecheck N           Macro: check elapsed time every N iterations (default: 256)
--csv FILE              Write results as CSV
```

## Examples

### Quick Sanity Check
```bash
./vittec-bench --iters 100000 --samples 3 micro:add micro:hash
```

### Comprehensive Testing
```bash
./vittec-bench --all --csv baseline.csv
```

### Focus on Memory Performance
```bash
./vittec-bench --all --filter "cache\|array\|memcpy"
```

### Deep Dive into Function Call Overhead
```bash
./vittec-bench --warmup 500 --samples 15 micro:call_inline micro:call_direct micro:call_deep
```

## Output Format

### Text (Default)
```
micro:add            mean=  123.45 ns  p50=  120 ns  p95=  145 ns  min=  100 ns  max=  200 ns
micro:hash           mean=  456.78 ns  p50=  450 ns  p95=  475 ns  min=  400 ns  max=  550 ns
macro:json_small     mean=    2.34 ms  p50=    2.3 ms p95=    2.5 ms min=    2.1 ms max=    3.0 ms
```

### CSV Format
```csv
id,mean,p50,p95,min,max
micro:add,123.45,120,145,100,200
micro:hash,456.78,450,475,400,550
macro:json_small,2.34,2.3,2.5,2.1,3.0
```

## Performance Tips

1. **Disable frequency scaling** for stable results:
   ```bash
   # macOS (if available)
   sudo pmset -b ttyskeepawake 0
   
   # Linux (performance mode)
   echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
   ```

2. **Increase sample count** for noisy measurements:
   ```bash
   ./vittec-bench --samples 20 micro:*
   ```

3. **Customize warmup** for different workloads:
   ```bash
   ./vittec-bench --warmup 5000 macro:json_large
   ```

4. **Use `--filter`** to compare specific patterns:
   ```bash
   ./vittec-bench --all --filter "cache" | grep -E "mean|max"
   ```

## Benchmark Categories

### Cache Hierarchy
- `micro:array_seq` — Sequential access (cache-friendly)
- `micro:array_s16` — Strided access (cache-unfriendly)
- `micro:array_rand` — Random access
- `micro:cacheline_*` — Cache line alignment effects
- `macro:cache_l1/l2/l3` — Working set sizes

### Branch & Prediction
- `micro:branch_predict` — Predictable branches
- `micro:branch_unpredict` — Unpredictable branches
- `micro:cmov_*` — Branch vs CMOV comparison

### ILP & Dependencies
- `micro:dep_chain_int` — Dependent integer chain
- `micro:dep_parallel` — Independent operations
- `micro:dep_load_chain` — Memory dependency

### String & Text Processing
- `micro:str_findchar_*` — Character search
- `micro:str_findsubstr_*` — Substring matching
- `macro:str_concat/search/case` — String operations
- `macro:regex_glob/email/url` — Pattern matching

### Computation
- `macro:matrix4/matrix10` — Matrix multiplication
- `macro:trig/log_exp/sqrt` — Math functions
- `macro:sort_bubble/insertion/quick` — Sorting algorithms

### Data Processing
- `macro:json_small/medium/large` — JSON parsing
- `macro:compress_rle/pattern` — Compression
- `macro:file_seq_read/rand_access` — I/O patterns

## Statistical Methodology

### Micro-benchmarks
- **Iteration count**: Fixed (default 1M per sample)
- **Sampling**: Multiple runs to measure variance
- **Statistics**: Mean, median (p50), p95, min/max
- **Summation**: Kahan algorithm (numerically stable)

### Macro-benchmarks
- **Duration**: Time-based (default 2s per sample)
- **Iteration count**: Varies based on workload
- **Time checking**: Periodic (every 256 iterations default)
- **Statistics**: Same as micro

## Adding New Benchmarks

### Create Micro-benchmark
```c
// src/micro/bm_example.c
#include "bench/bench.h"

static void bm_example_case1(void* ctx) {
  (void)ctx;
  // Benchmark body
  for (int iter = 0; iter < 100000; iter++) {
    // tight loop
  }
}

void bench_register_micro_example(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn fn, void* ctx);
  bench_registry_add("micro:example_case1", BENCH_MICRO, bm_example_case1, NULL);
}
```

### Register Benchmark
1. Add function call to `benchmark_init.c`:
   ```c
   extern void bench_register_micro_example(void);
   bench_register_micro_example();
   ```

2. Add source to `CMakeLists.txt`:
   ```cmake
   set(BENCH_MICRO_SOURCES ${BENCH_MICRO_SOURCES} src/micro/bm_example.c)
   ```

3. Rebuild: `cd build && make`

## Troubleshooting

### "unknown case" error
Check available benchmarks:
```bash
./vittec-bench --list
```

### Noisy results
- Disable background processes
- Increase sample count: `--samples 20`
- Increase warmup: `--warmup 5000`

### Compilation errors
Ensure C11 support and math library:
```bash
cmake -DCMAKE_C_STANDARD=11 ..
```

## Architecture

### Registry System
- Static array-based registry (no dynamic allocation)
- O(n) lookup by name
- Thread-safe read operations

### Timing Abstraction
- Platform-specific monotonic clocks
- Nanosecond precision
- No system call overhead

### Statistical Engine
- Kahan summation for numerical stability
- Percentile computation (linear search)
- Outlier detection (simple min/max tracking)

## Performance Characteristics

**Framework overhead**:
- Micro: ~5-10% of measured time
- Macro: <1% of measured time

**Memory usage**:
- Static allocations: ~1MB
- Dynamic (per sample): ~64KB

**Code size**:
- Total: ~56 files, ~2500 LOC
- No external dependencies

## License

Same as Vitte compiler. See LICENSE.

## Contributing

To add benchmarks:
1. Create `.c` file in `micro/` or `macro/`
2. Implement `bench_register_*()` function
3. Add to `benchmark_init.c`
4. Update `CMakeLists.txt`
5. Test with `./vittec-bench --list`

## Contact

For issues or suggestions, refer to main Vitte project documentation.
