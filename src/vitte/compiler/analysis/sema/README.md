# sema

Path: `src/vitte/compiler/analysis/sema`

## Purpose

Semantic analysis phases: resolve, sema, typeck, borrowck, lint.

## Notes

- Keep phase boundaries explicit and testable.
- Emit structured diagnostics with stable codes.
- Ensure fatal conditions stop pipeline immediately.

## Test State

- `make sema-analysis-test` checks the executable semantic-analysis unit surface.
- `make sema-fixtures` validates the fixture corpus under `tests/sema`.
- `make sema-snapshots` checks semantic diagnostics, module summaries, symbol tables, suggestions, ordering, multi-file cases, cycles, warning-only cases, and mixed error/warning snapshots.
- `make sema-coverage` validates the semantic coverage manifest and writes `target/reports/sema_coverage/coverage.json` plus `coverage.md`.
- `make sema-gate` runs all of the above and is part of `core-language-gate` and `compiler_max_gate.sh`.

## Fixture Layout

- `tests/sema/valid`: single-file valid semantic fixtures.
- `tests/sema/invalid`: focused invalid semantic fixtures.
- `tests/sema/multifile`: module graph fixtures for imports, visibility, and cycle contracts.

## Remaining Coverage

The readable remaining-work report is generated at `target/reports/sema_coverage/remaining.md`.
Current known gaps are real import-cycle graph detection, explicit missing-export validation, duplicate field/variant/trait-method diagnostics owned directly by sema, and compiler-emitted JSON snapshots for every fixture.
