# vitte/compiler bench

C17 microbench & macrobench harness for the Vitte compiler + runtime components.

Architecture:
- `bench_main.c`      : CLI entrypoint
- `bench_registry.c`  : case registry (explicit registration, portable)
- `bench_runner.c`    : runner + sampling + baseline compare
- `bench_time.c`      : high-resolution timers (portable)
- `bench_platform.c`  : cpu count + affinity best-effort
- `bench_stats.c`     : min/max/mean/stddev + percentiles
- `bench_report.c`    : stable text/json reporting
- `bench_csv.c`       : CSV reporting
- `bench_baseline.c`  : baseline regression checker
- `suites/`           : subsystem suites (lex/parse/ir/codegen/vm/alloc/gc)
- `data/`             : seed corpora and synthetic generators
- `scripts/`          : perf/baseline helpers
- `tests/`            : schema + fixtures

Usage:
- list benchmarks:
  `vitte-bench --list`
- run filtered:
  `vitte-bench --filter lex --iters 200 --warmup 20`
- deterministic-ish:
  `vitte-bench --pin-cpu 0 --flush-cache --seed 1`
- JSON output to file:
  `vitte-bench --format json --out bench.json`
- baseline regression check:
  `vitte-bench --baseline baseline.json --fail-above 5.0`
