# type_system

Path: `tests/type_system`

## Purpose

Compiler and repository test suites with fixtures and contracts.

## Notes

- Add focused regression tests for each bug fix.
- Prefer deterministic fixtures over random behavior.
- Assert phase/status contracts, not only success codes.
- `tools/typeck_differential_test.py` generates primitive, binding-backed and nominal assignments, returns and arguments, concrete and generic call-return inference, plus arity, generic inference, trait-bound, trait-implementation resolution, completeness, signatures, method dispatch, bound-method arity, arguments and return inference, coherence, constraint-cycle, cast, numeric-coercion, finite-match exhaustiveness, alpha-renaming, whitespace and condition cases, then compares normalized diagnostics across stage0 (`bin/vittec0`), stage1 (`bin/vittec1`) and stage2 (`bin/vittec`).
- Differential cases execute the CLI type-checking pipeline and validate JSON diagnostics; they do not rely on the compiler-suite shell bridge.
- Every negative differential case requires a non-empty two-step type-checking cause chain.
- `tools/typeck_fuzz_test.py` deterministically mutates seven source families and rejects crashes, timeouts, malformed JSON, fatal/internal diagnostics, non-determinism and cross-stage divergence.
