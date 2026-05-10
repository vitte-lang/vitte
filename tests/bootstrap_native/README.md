# Bootstrap Native Snapshots

This directory pins the `native_ir_v1` contract used by `build-native`.
`native_ir_v1` is the stable contract; the generated POSIX shell is the current
bootstrap backend and may be replaced as long as the IR contract remains stable.

`native_ir_v2` is an optional, additive preview contract used to prepare
backend-agnostic metadata (`target.*`, `artifact.*`) without breaking v1.

`proc.int.*` IR entries are record-only in v1. They are snapshot coverage for
integer procedure lowering, not callable commands in generated shell compilers.

## IR snapshots

`*.ir.must` files pin the deterministic `native_ir_v1` text emitted by
`dump-native-ir`. They cover the stage2 compiler entry and each accepted
bootstrap-native fixture.

`stage2.v2.ir.must` pins the current optional `native_ir_v2` additive contract.
The v1 and v2 contracts are validated independently.

## Diagnostic snapshots

`bad_*.err.must` files pin negative diagnostics. Each line is a required needle
instead of a whole stderr transcript, so the contract locks error code, source
location, and human-facing reason without depending on incidental shell output.

## CLI snapshots

`cli.*.err.must`, `parse.stage2.must`, `check.stage2.must`, and
`check.*.err.must` pin command-surface behavior for missing arguments,
unsupported bootstrap commands, parse parity output, and `check` success/failure
behavior.

`strict.check.*.err.must` pins strict-mode compatibility rules for bootstrap
entry files.

`trace.*.err.must` and `trace.*.out.must` pin trace ordering and failure
diagnostic ordering for `--trace-pipeline`.

Trace grammar is contractually fixed and mandatory:

`[trace] pipeline.<phase>:<event>`

Where `<phase>` is one of `parse`, `check`, `lower`, `emit`.
Any grammar drift must be intentional and snapshot-reviewed.

`unknown_command.<tool>.err.must` files pin the generated error prefix for each
stage binary. The prefix comes from the first word of `version_text()`.

## Help snapshots

`help.main_const_int.must` pins the exact generated `--help` text for the same
fixture, including banner, version line, and command list.

`help.vittec0.must`, `help.vittec1.must`, and `help.vittec.must` pin the exact
generated `--help` text for the seed, stage1 compiler, and stage2 compiler.

## Shell backend snapshots

`shell.*.must` files are human-readable backend snapshots from
`dump-native-shell`. They exist so backend changes are reviewable without
decoding only SHA-256 hash drift.

- `shell.named_consts.must` protects named string constants and version/banner
  propagation through the generated shell.
- `shell.main_proc.must` protects a literal `main.return` lowering path.
- `shell.main_const_int.must` protects named int constants and `main.return`
  through a constant.

## Emission hashes

`emission.sha256.must` is intentionally strict and contains only hash records,
one per emitted executable. The labels mean:

- `vittec1`: stage1 compiler emitted by `vittec0` from
  `toolchain/stage1/src/main.vit`.
- `vittec`: stage2 compiler emitted by `vittec1` from
  `toolchain/stage2/src/main.vit`.
- `named_consts`: fixture binary emitted from `named_consts.vit`; protects named
  string constant lowering and `--version` propagation.
- `main_proc`: fixture binary emitted from `main_proc.vit`; protects
  `proc main(args: list[string]) -> int` lowering and process exit behavior.
- `main_const_int`: fixture binary emitted from `main_const_int.vit`; protects
  integer constants used as `main.return` and generated `--help` text.

When any hash changes intentionally, update the matching IR/diagnostic snapshot
and document the reason in the change. Verify with:

```sh
make bootstrap-native-snapshots
```

## IR version gate

`tools/bootstrap_native_snapshots.sh` enforces an explicit gate:

- `dump-native-ir` default header must remain `native_ir_v1`
- `stage2.v2.ir.must` must exist and start with `native_ir_v2`

If either check fails, the snapshot run exits with a migration error requiring
explicit docs/snapshot updates.
