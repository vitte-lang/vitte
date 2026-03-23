# Test Strategy

This page explains the current testing strategy by layer.

The goal is not to run everything for every change.
The goal is to know which layer owns which regression risk.

## Layered Test Map

| Layer | Main concern | Representative checks |
| --- | --- | --- |
| parser / grammar | syntax acceptance, grammar artifacts, parser regressions | `make grammar-check`, `make grammar-gate`, `make parse` |
| frontend validation | diagnostics, imports, return analysis, early semantics | `make core-language-gate`, `make hir-validate`, `make diag-snapshots` |
| protected language contract | release-facing core guarantees | `make core-release-gate` |
| modules / packages | package shape, ownership, doctor/graph snapshots | `make modules-tests`, `make modules-snapshots`, `make modules-contract-snapshots`, `make packages-gate` |
| completions / CLI surface | generated shell completions and completion snapshots | `make ci-completions` |
| broader CI | grouped project-wide regression coverage | `make all-tests`, workflow matrix under `.github/workflows/all-tests.yml` |
| VitteOS track | OS-specific scripts, contracts, smoke checks | `make vitteos-quick`, `make vitteos-ci`, `make vitteos-ci-strict` |

## Selection Rule

Run the narrowest sufficient gate first.

Examples:

- syntax change: `make parse` then `make core-language-gate`
- diagnostic wording or import behavior: `make hir-validate` then `make core-language-gate`
- release-facing core wording or guarantees: `make core-release-gate`
- package governance changes: modules/package gates
- completion changes: `make ci-completions`

## Why This Matters

The repository is large enough that undirected testing wastes time.
Layered testing keeps changes scoped and makes failures easier to interpret.
