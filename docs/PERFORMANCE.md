# Performance

This document defines the minimum performance baseline story Vitte should expose publicly.

The immediate goal is not sophisticated benchmarking.
The immediate goal is a visible, repeatable baseline.

## Reporting Rule

Performance reporting should answer four questions every time:

1. what was measured
2. on which machine profile
3. with which command/configuration
4. whether the result is within the accepted noise band

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

## Measurement Protocol

Use the same protocol for every published baseline:

1. start from a clean working tree for measured files
2. record commit hash
3. record machine profile
4. record compiler/toolchain versions
5. warm up once if needed
6. run the measured command at least 5 times
7. report median first, then min/max if helpful
8. keep CI measurements separate from local developer measurements

If the command writes outputs, use the same target path structure across runs.

## Reference Machine Profiles

The project does not need a lab to become credible.
It needs named profiles.

Recommended initial profiles:

| Profile | Purpose | Example environment |
| --- | --- | --- |
| `dev-linux` | contributor workstation baseline | Ubuntu/Debian, local SSD, standard clang toolchain |
| `dev-macos` | contributor workstation baseline on macOS | Apple Silicon macOS with Homebrew toolchain |
| `ci-linux` | regression visibility | GitHub Actions `ubuntu-latest` or equivalent |

Numbers from different profiles should never be compared as if they were the same track.

## Variability Thresholds

Use simple thresholds before adding hard perf gates:

| Metric type | Expected noise band | Escalate when... |
| --- | --- | --- |
| small example wall-clock time | within 5% | regression persists above 10% |
| medium/large example wall-clock time | within 7% | regression persists above 12% |
| binary or object size | within 1% | regression persists above 3% |
| package bench smoke | benchmark-specific | benchmark exceeds its documented threshold |

These thresholds are intentionally conservative.
They are for triage first, not for pretending perfect determinism.

## Measurement Rules

- always record machine profile when reporting numbers
- separate developer-machine numbers from CI numbers
- compare like with like: same compiler, same flags, same target
- prefer trends over isolated single-run claims
- do not present experimental numbers like hard guarantees

## Suggested First Commands

The first visible measurements can stay simple:

```sh
time bin/vitte parse examples/first_project.vit
time bin/vitte check examples/first_project.vit
time bin/vitte build examples/first_project.vit
time bin/vitte build examples/syntax_features.vit
time bin/vitte build examples/enorme_10k.vit
```

If object size is being tracked, keep the target and flags fixed for every report.

## Public Reporting Goal

The next useful step is a small recurring report that shows:

- current baseline numbers
- previous baseline numbers
- notable regressions
- measurement environment

That is enough to create accountability without pretending the benchmark system is finished.
