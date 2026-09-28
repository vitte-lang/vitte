# Compiler Power Roadmap

This document defines the engineering path for making Vitte competitive with mature production compilers such as Rust, C#, and Java.

The goal is not feature volume. The goal is a compiler whose behavior is specified, checked, diagnosed, optimized, and integrated well enough that users can trust it on large codebases.

## Maturity Model

### L0: Syntax and Diagnostics

- Deterministic lexer and parser.
- Structured diagnostics with stable codes.
- Human, JSON, and LSP output for diagnostics.
- Snapshot coverage for common user-facing errors.
- Recovery that prefers the first root cause over cascades.

Gate:

```sh
make diagnostic-quality
make compiler-test-suite-check-gate
```

### L1: Semantic Core

- Symbol resolution with module visibility rules.
- Type checking with expected/found provenance.
- Generic and trait/interface constraints represented explicitly.
- Borrow, move, lifetime, and alias checks with structured explanations.
- Constant evaluation and lint passes integrated into the same report model.

Gate:

```sh
make compiler-test-suite-check-gate
make type-system-gate
```

### L2: Verified Middle End

- HIR and MIR validation before and after transformations.
- MIR lowering with stable control-flow and place semantics.
- Dataflow, liveness, and ownership facts with regression tests.
- Optimization passes that validate input and output.
- Golden IR/MIR snapshots for representative programs.

Gate:

```sh
make compiler-test-suite-check-gate
make analysis-gate
```

### L3: Backend and Runtime

- Portable backend contract for C/LLVM/native paths.
- ABI profiles and explicit calling convention rules.
- Debug-info and symbol-output policy.
- Linker errors mapped to structured diagnostics.
- Runtime object/materialization checks in the driver pipeline.

Gate:

```sh
make backend-gate
```

### L4: Tooling and Developer Experience

- LSP diagnostics and code actions.
- Formatter and stable style rules.
- Package/module checks.
- Test runner and benchmark runner.
- Documentation generator tied to compiler contracts.

Gate:

```sh
make diagnostic-contracts
make compiler-test-suite-check-gate
make compiler-power-gate
```

### L5: Production Reliability

- Full compiler gate in CI.
- Stress, chaos, fuzz, and negative tests.
- Incremental compilation invalidation checks.
- Cross-platform packaging and bootstrap reproducibility.
- Release gates that fail on unowned diagnostics or unchecked compiler paths.

Gate:

```sh
make compiler-max-gate-fast
make compiler-gate
```

## Current Focus

The next high-leverage track is `compiler-power-gate`: a focused, fast gate for the semantic core, diagnostics, HIR/MIR, backend smoke, and driver pipeline.

It is intentionally smaller than `compiler-gate` and stricter than a single `bin/vitte check` pass. It should stay fast enough to run during normal compiler work.

## Acceptance Criteria

- Every public diagnostic has a stable code, explanation, help, and suggestion path.
- Type errors expose expected/found context and at least one repair path.
- Borrow errors expose the move/borrow timeline and a repair path.
- HIR/MIR tests prove lowering preserves control flow, projection, and validity.
- Backend tests prove MIR-to-IR/codegen contracts do not silently regress.
- Pipeline tests prove JSON surfaces and stop-after modes remain stable.

## Expansion Backlog

- Add generic constraint tests with success and failure cases.
- Add trait/interface resolution tests once the trait model is stable.
- Add lifetime-region snapshot tests for nested scopes and returns.
- Make `lexer_tests.vit`, `borrowck_tests.vit`, `const_eval_tests.vit`, and `codegen_tests.vit` checkable by the current parser surface.
- Add MIR optimization before/after validation snapshots.
- Add ABI conformance fixtures for exported functions and FFI calls.
- Add LSP code-action JSON snapshots for diagnostics with machine suggestions.
- Add formatter/idempotence tests when formatting is implemented.

## Known Checkability Gaps

Some compiler test suites are intentionally not part of `compiler-power-gate` yet because they contain fixture strings that expose current parser/check limitations:

- Multi-line string concatenation after `=` or `+` is rejected as an incomplete expression.
- Certain fixture text inside string literals is still interpreted by early validation paths, for example numeric-looking fragments and `static_assert(...)` snippets.
- These are parser/check-surface issues, not reasons to weaken semantic coverage. The stable gate uses `compiler-test-suite-check-gate` until these suites are migrated or the parser accepts the fixture style.
