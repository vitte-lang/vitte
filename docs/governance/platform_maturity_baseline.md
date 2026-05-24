# 215. Vitte Platform Maturity Baseline

Objective: define the minimum evidence required to claim credible platform maturity.

Date: 2026-05-24

## Maturity Conditions

Each condition below must have objective evidence.

1. Progressively self-hostable compiler
- Evidence: staged bootstrap artifacts, reproducible stage progression checks, documented trust root.

2. Verifiable pipeline
- Evidence: deterministic step outputs, pipeline tests per stage, gate reports.

3. Reliable diagnostics
- Evidence: stable code catalog, snapshot coverage, deterministic output under strict mode.

4. Reproducible backend
- Evidence: same-source/same-toolchain identical outputs for covered targets.

5. Stable runtime
- Evidence: runtime smoke + stress + fault-path tests and regression tracking.

6. Maintainable stdlib
- Evidence: ownership map, API contracts, docs/tests sync, deprecation tags.

7. Stable IDE tooling
- Evidence: LSP smoke checks, completion/hover/diag contracts and snapshots.

8. Reproducible package management
- Evidence: lockfile contract, dependency resolution determinism, offline/install replay checks.

9. Robust incremental compilation
- Evidence: cache hit/miss reports, invalidation tests, no stale-result regressions.

10. Validated deterministic builds
- Evidence: deterministic build gates in CI + release validation report.

11. Active chaos testing
- Evidence: periodic fault-injection runs with tracked outcomes and fixes.

12. Active fuzzing
- Evidence: continuous parser/frontend fuzz runs and corpus growth tracking.

13. Active stress testing
- Evidence: sustained workload tests with thresholds and trend history.

14. Documented architecture
- Evidence: architecture pages aligned with source and checked for drift.

15. Defined technical governance
- Evidence: approved governance docs and review cadence in operation.

16. Possible long-term maintenance
- Evidence: support windows, deprecation and migration policy, triage SLO adherence.

## Baseline Scorecard

Status rubric:

- `red`: no reliable evidence or repeated regressions.
- `amber`: partial evidence; gaps block platform claim.
- `green`: evidence automated and passing in release gates.

A release can be labeled "platform-credible baseline met" only when all 16 conditions are `green` or when temporary `amber` exceptions are explicitly approved in release notes with deadlines.
