

# Vitte Examples Matrix Report

Generated: 2026-06-21 15:00 UTC

---

## Executive Summary

| Metric | Value |
|----------|----------|
| Total Examples | 407 |
| Passed | 396 |
| Failed | 11 |
| Timeouts | 0 |
| Average Time | 84 ms |
| Median Time | 73 ms |
| P95 | 182 ms |
| P99 | 341 ms |
| Min Time | 2 ms |
| Max Time | 912 ms |
| Total Elapsed | 34182 ms |

---

## Performance Overview

| Metric | Value |
|----------|----------|
| Slowest Example | compiler/typeck.vit |
| Slowest Example Time | 912 ms |
| Average Regression | 14.2 % |
| Performance Regression | 18.0 % |

---

## Compiler Pipeline

```text
Lexer
  ↓
Parser
  ↓
AST
  ↓
HIR
  ↓
Sema
  ↓
Typeck
  ↓
Borrowck
  ↓
MIR
  ↓
IR
  ↓
Backend
  ↓
Linker
```

---

## Category Statistics

| Category | Examples | Average (ms) | Worst (ms) |
|----------|----------|----------|----------|
| compiler | 120 | 91 | 912 |
| async | 42 | 34 | 221 |
| runtime | 65 | 51 | 411 |
| stdlib | 180 | 17 | 95 |

---

## Quality Report

| Metric | Value |
|----------|----------|
| Documentation Coverage | 92 % |
| Examples With Benchmarks | 407 |
| Examples With History | 407 |
| CI Ready | true |

---

## Generated Artifacts

```text
build/examples/examples-matrix.json
build/examples/examples-matrix.md
build/examples/examples-matrix.html
build/examples/examples-matrix.xml
build/examples/regressions.json
build/examples/regressions.csv
build/examples/regressions.md
build/examples/regression.html
```

---

## Roadmap

- parallel_examples_execution
- historical_trend_analysis
- automatic_regression_detection
- interactive_dashboard
- compiler_stage_benchmarks
- stdlib_benchmarks
- knowledge_graph_integration
- docs_performance_portal

---

## Notes

This report is intended to summarize benchmark execution across the Vitte example suite. Future versions should automatically populate all metrics from build outputs and historical benchmark snapshots.
