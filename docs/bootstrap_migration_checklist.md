# Bootstrap Migration Checklist (toward 100% Vitte)

Goal: remove temporary host-language audit exceptions once stage1/stage2 are fully Vitte.

## Phase 0 — Baseline (current)

- [x] `make seed-gate` is stable on `driver/ir/frontend/backends` + valid tests.
- [x] `make bootstrap-all` runs end-to-end locally.
- [x] `make bootstrap-verify` validates versions + smoke + AST/IR checks.
- [x] `vitte-source-audit` runs with no host-language source exceptions.

Exit criteria:
- All baseline commands pass on local and CI runs.

## Phase 1 — Stage1 becomes Vitte-native

- [x] Replace `toolchain/stage1` host stub implementation with Vitte sources only.
- [x] Ensure `stage1.sh` rebuilds `bin/vittec1` from `toolchain/stage1/src/main.vit` using `vittec0`.
- [x] Add/extend smoke tests for stage1 output consistency.
- [x] Keep stage0 as a checked seed artifact, not as repo-hosted C source.

Exit criteria:
- `stage1.sh` succeeds without any host-language source under `toolchain/stage1`.
- `bin/vittec1 --version` reports stage1-native identity.

## Phase 2 — Stage2 becomes Vitte-native

- [x] Replace `toolchain/stage2` host stub implementation with Vitte sources only.
- [x] Ensure `stage2.sh` produces `bin/vittec` from stage1 compiler output.
- [x] Validate stage2 self-check path (`VITTE_SELF_CHECK=1`) with deterministic behavior.
- [x] Install stage2 as `bin/vitte` and pass full `make build`.

Exit criteria:
- `stage2.sh` succeeds without any host-language source under `toolchain/stage2`.
- `bin/vitte --version` reports stage2-native identity.

## Phase 3 — Tighten audit exceptions

- [x] Remove the legacy stage0 source exception from `vitte-source-audit`.
- [x] Keep CI green (`seed-gate`, `bootstrap-all`, `build`) after removing exceptions.
- [x] Remove dead host-only scripts no longer needed by stage1/stage2.

Exit criteria:
- `make build` passes with no host-language source exceptions.

## Phase 4 — Remove stage0 C exception (final)

- [x] Introduce a trusted prebuilt bootstrap seed path.
- [x] Retire the legacy stage0 source tree from mandatory flow.
- [x] Remove remaining host-language exception from `vitte-source-audit`.
- [x] Ensure CI passes with zero repo-hosted host-language sources required.

Exit criteria:
- `vitte-source-audit` passes with no host-language exceptions.
- Bootstrap flow is fully Vitte-native and reproducible.

## Operational guardrails during migration

- Keep `seed-compat-report` tracked in CI artifacts.
- Keep `docs/bootstrap_native_ir.md` aligned with every `build-native` subset extension.
- Treat any new host-language file outside approved exception scope as a hard failure.
- Prefer adding targeted regression tests before broad refactors.
- Update `README.md` whenever a phase exit criterion is completed.
