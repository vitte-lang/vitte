# mir

Path: `src/vitte/compiler/middle/mir`

## Purpose

Core project directory.

## Generic instances

- `MirFunction.generic_params` preserves the declaration order from typed HIR.
- `MirRvalue.generic_arguments` carries concrete call arguments in that same order.
- MIR validation substitutes call arguments into parameter types before checking the call contract.
- `lower_hir_to_mir_templates` exposes the transient template MIR for focused tests.
- `monomorphize_mir` deduplicates concrete instances, substitutes signatures and bodies, and removes templates before canonical MIR validation.
- Concrete symbols use deterministic `vitte_mono_*` names; source spelling is not the generic identity.

## Optimization invariants

- The production schedule lives under `middle/passes`; metric-only optimization fixtures are not part of the compiler pipeline.
- `run_passes` validates canonical MIR before and after every scheduled transform and stops at the first invalid boundary.
- Only a successfully validated transformed unit is forwarded to backend IR lowering.

## Coverage state

MIR coverage is tracked by `src/vitte/compiler/tests/mir_coverage_manifest.json` and checked by `tools/mir_coverage_check.py`.

The current gate requires every declared supported MIR surface to have:

- a manifest entry;
- a `support_status`;
- an `owner_phase`;
- an evidence test;
- a concrete assertion for that evidence.

Generated reports are written to `target/reports/mir_coverage/`:

- `coverage.json` contains the machine-readable gate result;
- `coverage.md` contains the review table;
- `remaining.md` lists missing entries and hardening tasks;
- `fixtures.md` records required MIR fixture artifacts and source fixtures.

## Test commands

- `make mir-lowering-test` runs the executable MIR lowering and validation suite.
- `make mir-fixtures` validates required MIR fixture artifacts.
- `make mir-snapshots` validates coverage snapshots.
- `make mir-coverage` validates the manifest and regenerates reports.
- `make mir-gate` runs the full MIR gate.

`mir-gate` is the reference local command before changing HIR-to-MIR lowering, MIR validation, monomorphization, pretty printing, dataflow or MIR transforms.

## Gate invariants

- Function, block, statement and temporary order must remain deterministic.
- MIR spans should preserve real HIR spans and only become synthetic when the source node was synthetic.
- `function_count`, `block_count`, `instruction_count`, `local_count` and `temporary_count` must match the emitted MIR.
- Every block must have a valid terminator and all CFG edges must target existing blocks.
- Validation must reject malformed calls, borrows, dispatch targets, generic arity and invalid assignment places.
- Monomorphized symbols must be deterministic and duplicate concrete instances must be deduplicated.
- `run_passes` must validate before and after each transform and stop at the first invalid boundary.

## Remaining work

- Expand compiler-emitted JSON snapshots beyond the current representative MIR fixture set.
- Add executable checks for every `tests/mir` source fixture once the CLI exposes stable per-fixture MIR dumps.
- Add structured MIR diagnostic snapshots when MIR validation emits diagnostics directly.
- Audit legacy `src/vitte/compiler/ir/mir_extended.vit` and `src/vitte/compiler/ir/hir_to_mir_lowering.vit` against canonical middle MIR.
