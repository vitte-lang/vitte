# Public Benchmark Methodology

This document defines how Vitte benchmark claims are produced for publication.

## Goals

- reproducible commands
- machine-readable outputs
- explicit pass/fail thresholds
- no marketing wording without measurements

## Inputs

The public dashboard consumes only generated artifacts:

- `target/bench/lsp_completion.latest.json`
- `target/bench/dx_hello_prod.latest.json`
- `target/reports/competitive/runtime_native_pgo.json`
- `target/reports/release_doctor.json`

## Commands

Run in this order:

```sh
make build-fast
make dx-adoption
make runtime-native-pgo-bench
make public-benchmark-dashboard
make release-proof-notes
```

## KPI Rules

The target is: **3 use cases where Vitte is clearly preferable**.

Current use-case rules:

1. local completion latency: `p95 <= 50 ms` and non-empty completion hits
2. hello->prod DX: `>= 40%` reduction versus configured baseline
3. runtime native PGO: `>= 1.05x` median speedup versus release baseline

The dashboard marks each rule `met=true/false` and computes total KPI status.

## Publication Constraints

- publish only with `target/reports/public_benchmark_dashboard.md`
- include `target/reports/release_notes_proof.md`
- do not claim wins for missing data points
- if a rule fails, publish as a limitation, not as a success
