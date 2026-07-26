# Bootstrap Seed Contract

The bootstrap seed is a historical stage0 artifact. It is kept only as an
offline bootstrap fallback for reconstructing an initial compiler in an empty
environment. It is not the active source for compiler development.

## Files

- Canonical readable source: `toolchain/seed/src/main.vit`
- Audited executable artifact: `toolchain/seed/vittec0.seed`
- Pinning manifest: `toolchain/seed/manifest.txt`
- Frozen policy: `toolchain/seed/frozen.json`

`toolchain/seed/manifest.txt` records the seed source path, artifact path,
artifact SHA-256, expected `--version` output, and frozen status. `make
seed-verify` checks the manifest hash against the artifact and validates the
version string.

`toolchain/seed/frozen.json` pins the current seed hashes and marks the seed as
`historical-frozen` with `bootstrap-fallback-only` usage. `make
seed-frozen-gate` enforces that policy.

## Change Rule

Normal development must not modify:

- `toolchain/seed/vittec0.seed`
- `toolchain/seed/src/main.vit`
- `toolchain/seed/manifest.txt`
- `toolchain/seed/frozen.json`

Any change to those paths fails `make seed-frozen-gate` in CI after the frozen
policy exists on the target branch. An intentional seed rotation is outside the
normal CI path and requires an explicit bootstrap plan plus
`SEED_FROZEN_OVERRIDE=1`.

## Fallback Use

The normal compiler path is `src/vitte/compiler` plus `src/vitte/stdlib`.
`vittec0.seed` may be used only to reconstruct a fallback stage0 when no
compiled Vitte compiler is available.

`make seed-rotation-report` now reports frozen status and hash identity. It does
not authorize rotation.

## Local Checks

```sh
make seed-frozen-gate
make seed-verify
make seed-rotation-report
make posix-seed-shell-check
make bootstrap-native-snapshots
make seed-contract-check
make bootstrap-native-drift-check
make bootstrap-posix-smoke
```

`make build` also runs bootstrap integration gates before the final source audit:

- `compiler-real-native-gate`
- `compiler-test-suite-check-gate`
- `compiler-no-fallback-gate`
- `driver-native-json-surface-gate`

These are complementary to `make bootstrap-native-snapshots`. The snapshot suite
locks seed output and diagnostics; the integration gates verify
that the current driver still exposes the expected native JSON envelopes, that
the compiler entry builds without a bridge sidecar, and that the compiler test
suite bridge remains scoped to compiler test sources.

## Local Parallelism

CI jobs run in isolated workspaces, so their bootstrap artifacts do not collide.
In one local worktree, bootstrap targets that install the seed are not
parallel-safe because they rewrite shared `bin/vittec0`.

The single-workspace runner rule is:

- `make bootstrap-native-snapshots` installs the seed and mutates
  `bin/vittec0`.
- `make bootstrap-verify` reinstalls and verifies `bin/vittec0`.
- `make bootstrap-posix-smoke` depends on `bootstrap-all`, so it also rebuilds
  the seed artifact before running POSIX checks.
- `make bootstrap-native-contract` composes those flows and should be treated as
  an exclusive bootstrap runner in a local worktree.

`tools/bootstrap_native_snapshots.sh` and `tools/bootstrap_posix_smoke.sh` use
per-run temporary directories under `target/` and clean them on exit, but the
shared `bin/vittec*` artifacts are intentionally not isolated. Avoid running
bootstrap targets concurrently in the same worktree.

## Source Coverage

The readable seed and real compiler entry should exercise the current
bootstrap-native forms they rely on:

- `toolchain/seed/src/main.vit` must include named string constants, named int
  constants, `proc main(args: list[string]) -> int`, and a named constant return.
- `src/vitte/compiler/main.vit` must include named `VERSION_TEXT` and
  `BANNER_TEXT` string constants, use a named string constant in
  `version_text()`, and define `main`.

This is checked by `make bootstrap-source-coverage-check` and is included in the
bootstrap-native contract targets.

The stricter trust-root syntax contract is documented in
`docs/compiler/selfhost_core_subset.md` and enforced by
`make selfhost-subset-check`.

## Project Path Resolution

Bootstrap and driver flows now accept a project directory as the CLI input path.
When a command such as `./bin/vitte check .` or `./bin/vitte build . -o
target/app` receives a directory-like path, it resolves the entry in this order:

1. src/main.vit
2. `src/vitte/compiler/main.vit`
3. `main.vit`

This keeps bootstrap-oriented projects and the compiler source tree addressable
through the same CLI surface.

For CI or release checks, `SEED_CONTRACT_BASE` may be set to the base commit or
branch used for the diff:

```sh
SEED_CONTRACT_BASE=origin/main make seed-contract-check
```

## AWK Native Parser Contract

`toolchain/seed/vittec0.seed` contains a small AWK parser inside
`compile_native_ir()`. This parser is not a general Vitte parser. Its contract is
to recognize only the bootstrap-native subset documented in
`docs/bootstrap_native_ir.md`, emit deterministic `native_ir_v1` text, and fail
closed on anything outside that subset.

Review rules for the AWK block:

- Keep accepted forms explicit and line-oriented.
- Prefer adding a fixture before widening a regular expression.
- Preserve source declaration order for `const.*` and `proc.int.*` records.
- Set diagnostic columns from the original raw line, not from the trimmed line.
- Any parser change must update at least one `.ir.must` or `.err.must` snapshot.
- Any generated shell behavior change must update the `shell.*.must` snapshots.

Coverage can be inspected with:

```sh
tools/bootstrap_native_fixture_matrix.sh
```
