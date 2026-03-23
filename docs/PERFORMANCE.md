# Performance

This document defines the minimum performance baseline story Vitte should expose publicly.

The immediate goal is not sophisticated benchmarking.
The immediate goal is a visible, repeatable baseline.

## What Should Be Tracked

At minimum, track these metrics:

1. compile time for a small example
2. compile time for a medium example
3. compile time for a larger stress example
4. output binary or object size for a fixed target
5. a few package-specific benches where they already exist

Repository candidates already present:

- `examples/first_project.vit`
- `examples/syntax_features.vit`
- `examples/enorme_10k.vit`
- package bench scripts under `tools/`

## Existing Repository Hooks

The repository already contains performance-oriented scripts such as:

- `tools/core_bench.sh`
- `tools/db_bench.sh`
- `tools/fs_bench.sh`
- `tools/http_bench.sh`
- `tools/http_client_bench.sh`
- `tools/json_bench.sh`

That is a good base.
What is missing is a public summary and a standard baseline set.

## Baseline Recommendation

Keep the first public baseline intentionally small:

| Metric | Suggested fixture |
| --- | --- |
| parse/check/build latency | `examples/first_project.vit` |
| build latency on syntax-focused sample | `examples/syntax_features.vit` |
| build latency on larger stress sample | `examples/enorme_10k.vit` |
| object or binary size | one fixed repro or build target |
| package bench smoke | one bench each for core/package families already instrumented |

## Measurement Rules

- always record machine profile when reporting numbers
- separate developer-machine numbers from CI numbers
- compare like with like: same compiler, same flags, same target
- prefer trends over isolated single-run claims
- do not present experimental numbers like hard guarantees

## Public Reporting Goal

The next useful step is a small recurring report that shows:

- current baseline numbers
- previous baseline numbers
- notable regressions
- measurement environment

That is enough to create accountability without pretending the benchmark system is finished.
