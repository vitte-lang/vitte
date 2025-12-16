# Vitte Benchmark Guide

## Overview

`vitte-bench` is a lightweight benchmarking harness designed to measure the performance of Vitte compiler components and runtime functions.

## Architecture

### Components

- **benchlib**: Core library providing timing, statistics, and registry
- **benchc**: Benchmark runner executable
- **Micro benchmarks**: Fast, unit-level operations (nanoseconds)
- **Macro benchmarks**: Larger workloads (milliseconds to seconds)

## Building

### Using CMake (Recommended)

```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j
```

### Using Makefile

```bash
make build    # Build the benchmarks
make clean    # Clean build artifacts
```

### Using build.sh

```bash
./build.sh
```

## Running Benchmarks

### List Available Benchmarks

```bash
./build/benchc --list
./build/benchc --list-full
```

### Run All Benchmarks

```bash
./build/benchc --all
```

### Run Specific Benchmarks

```bash
./build/benchc micro:add
./build/benchc micro:hash micro:memcpy
./build/benchc macro:json_parse
```

### Run with Custom Parameters

```bash
# Custom iterations for micro benchmarks
./build/benchc --iters 5000000 micro:add

# Custom duration for macro benchmarks
./build/benchc --seconds 5 macro:json_parse

# Run with N samples
./build/benchc --samples 10 micro:hash

# Enable warmup iterations
./build/benchc --warmup 5000 micro:memcpy
```

### Export Results

```bash
# Save results as CSV
./build/benchc --csv results.csv --all

# Pipe to other tools
./build/benchc --all | tee benchmark.log
```

## Benchmark Types

### Micro Benchmarks

Located in `src/micro/`, these measure small, fast operations:

- **bm_add**: Integer addition throughput
- **bm_hash**: FNV-1a hash function throughput
- **bm_memcpy**: Memory copy performance

Typical durations: **nanoseconds per operation (ns/op)**

### Macro Benchmarks

Located in `src/macro/`, these measure larger workloads:

- **bm_json_parse**: JSON parsing throughput

Typical durations: **milliseconds per iteration (ms/op)** or **operations per second**

## Understanding Results

### Statistics

Each benchmark produces:

- **Mean**: Average of all samples
- **Median (p50)**: 50th percentile
- **p95**: 95th percentile (upper tail)
- **Min**: Best sample
- **Max**: Worst sample

### Example Output

```
micro:add
  samples: 7
  mean:    12.34 ns/op
  median:  12.10 ns/op
  p95:     13.20 ns/op
  min:     11.90 ns/op
  max:     14.50 ns/op
```

## Configuration Options

### CMake Flags

```bash
# Enable extra (opt-in) benchmarks
cmake -DVITTE_BENCH_EXTRA=ON ..

# Enable experimental benchmarks
cmake -DVITTE_BENCH_EXPERIMENTAL=ON ..

# Use optimized ASM implementations
cmake -DVITTE_BENCH_USE_ASM_MEMCPY=ON ..
cmake -DVITTE_BENCH_USE_ASM_HASH=ON ..

# Link against parent vitte_core target
cmake -DVITTE_BENCH_LINK_VITTE_CORE=ON ..
```

## Adding New Benchmarks

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on adding new benchmarks.

## Performance Tips

1. **Disable System Activity**: Close other applications during benchmarking
2. **Use Release Build**: Always benchmark with `-DCMAKE_BUILD_TYPE=Release`
3. **Multiple Runs**: Vary iterations/samples to find stable measurements
4. **Warm Up**: Benchmarks include warmup by default; adjust with `--warmup`
5. **Statistical Significance**: p95 should be close to median for reliable results

## Troubleshooting

### High Variance (p95 >> median)

- System is busy; close other applications
- Increase `--iters` for micro benchmarks
- Increase `--seconds` for macro benchmarks

### Impossible Values

- Verify hardware supports required CPU instructions
- Check if optimization flags are applied (Release mode)
- Ensure timing granularity is sufficient for your operation

### Linking Errors

- For custom ASM implementations, ensure symbols are available
- Check `-DVITTE_BENCH_LINK_VITTE_CORE=ON` if using parent target

## References

- [benchmark.h](src/bench/bench.h) — API documentation
- [CONTRIBUTING.md](CONTRIBUTING.md) — Adding benchmarks
- [CMakeLists.txt](CMakeLists.txt) — Build configuration
