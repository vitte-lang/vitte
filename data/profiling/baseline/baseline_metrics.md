# Baseline Metrics

- Total benchmarks: 5
- PASS: 5
- FAIL: 0

## Results
- async_spawn_10k: target < 100 ms, measured 82.4 ms -> PASS
- thread_pool_throughput: target < 50 us_per_task, measured 44.1 us_per_task -> PASS
- string_concat_1gb: target < 2 s, measured 1.78 s -> PASS
- json_serialize_1m: target < 500 ms, measured 463.7 ms -> PASS
- http_latency: target < 5 ms, measured 4.21 ms -> PASS

Overall: PASS
