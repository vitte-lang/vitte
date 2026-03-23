# Machine-Readable Outputs

This page defines the repository rule for machine-readable outputs.

## Purpose

As the tool surface grows, some outputs need to be stable enough for scripts, CI, or reports to consume.

This document does not claim every command output is stable JSON.
It defines where that expectation is appropriate.

## Good Candidates

Machine-readable outputs should be preferred for:

- reports written under `target/reports`
- generated indexes
- baseline and budget reports
- contract and compatibility summaries

Examples already present in the repository:

- `target/reports/index.json`
- `target/reports/release_doctor.json`
- `target/reports/repro.json`
- `tools/perf_baseline.dev.json`
- `tools/perf_baseline.ci.json`
- `tools/perf_budget.json`

## Rules

When a tool claims machine-readable output:

1. prefer JSON
2. keep field names stable
3. include a schema or version field when the output is important across time
4. document where the file is written
5. avoid mixing human log chatter into the JSON stream

## Human vs Machine

For important tools, the best pattern is:

- human-readable summary on stdout
- machine-readable JSON written to a predictable file

That keeps local use pleasant and CI/reporting reliable.
