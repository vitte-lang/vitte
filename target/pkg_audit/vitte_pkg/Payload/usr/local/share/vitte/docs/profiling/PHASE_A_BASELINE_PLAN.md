# Phase A Baseline Profiling (Week 1-2)

## Tools
- perf
- flamegraph
- cargo-flamegraph

## Benchmark suite
- async_spawn_10k (target < 100ms)
- thread_pool_throughput (target < 50us/task)
- string_concat_1gb (target < 2s)
- json_serialize_1m (target < 500ms)
- http_latency (target < 5ms)

## Deliverables
- `data/profiling/baseline/bench_results.csv`
- `data/profiling/baseline/baseline_metrics.md`
- `data/profiling/baseline/hot_functions_top5.md`
