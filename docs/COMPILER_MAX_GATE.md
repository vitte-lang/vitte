# Compiler Max Gate (1 -> 15)

This document maps the 15 max-compiler goals to concrete checks executed by `tools/compiler_max_gate.sh`.

1. Frontend golden snapshots: `make test-golden` / `make test-golden-critical`.
2. Fuzz lexer/parser: `tools/parser_lexer_fuzz_smoke.py`.
3. Hard invariants after passes: `tools/pass_contract_audit.py` + existing validators in driver pipeline.
4. Pass manager/ordering contracts: `tools/pass_contract_audit.py`.
5. Diagnostics quality regressions: `make core-semantic-resolve-snapshots` (fast), `make diag-snapshots` (full, with fallback to core-semantic-resolve-snapshots).
6. Parser recovery strictness: `tools/strict_recovery_smoke.sh`.
7. Precedence/associativity matrix: `tools/parser_precedence_property_test.py`.
8. Strict modes: `make strict-core-guard-test` / `make core-language-test`.
9. Deterministic outputs: `tools/determinism_smoke.sh` (fast), `make same-output-hash` (full, with fallback smoke).
10. Incremental/cache safety: `tools/incremental_cache_smoke.sh` (fast), `make ci-fast-compiler` (full with fallback to `tools/incremental_cache_smoke.sh`).
11. Perf baseline: `make profile-sample` (full mode).
12. ABI/interop guard: `make extern-abi-host` / `make extern-abi-all`.
13. Differential stability: covered by deterministic hash + golden snapshots.
14. Crash triage regressions: `tools/crash_report_snapshots.sh` (fast), `make all-tests-group GROUP=core` (full with fallback to `tools/crash_report_snapshots.sh`).
15. Unified release bar: `make compiler-max-gate-fast` / `make compiler-max-gate`.

## Commands

- Fast local gate: `make compiler-max-gate-fast`
- Full gate: `make compiler-max-gate`
