# Bootstrap Seed Contract

The bootstrap seed is the trusted stage0 entry point for rebuilding the Vitte
toolchain without repo-hosted host-language sources.

## Files

- Canonical readable source: `toolchain/seed/src/main.vit`
- Audited executable artifact: `toolchain/seed/vittec0.seed`
- Pinning manifest: `toolchain/seed/manifest.txt`

`toolchain/seed/manifest.txt` records the seed source path, artifact path,
artifact SHA-256, and expected `--version` output. `make seed-verify` checks the
manifest hash against the artifact and validates the version string.

The readable source is the canonical intent for review. The executable artifact
is the stage0 trust root actually run by bootstrap scripts. `make seed-verify`
keeps the two tied at the identity boundary by checking that source
`VERSION_TEXT`, manifest `version=...`, and `vittec0.seed --version` match.

## Change Rule

Any intentional seed rotation must update these contracts together:

- `toolchain/seed/src/main.vit` when the readable seed behavior changes.
- `toolchain/seed/vittec0.seed` when the executable seed artifact changes.
- `toolchain/seed/manifest.txt` when the artifact bytes or version change.
- `tests/bootstrap_native/*` snapshots when generated native output changes.
- CI must pass, including `make seed-verify` and
  `make bootstrap-native-snapshots`.

The seed artifact must not change on its own. A change to
`toolchain/seed/vittec0.seed` without a matching `toolchain/seed/manifest.txt`
change is treated as seed drift and fails `make seed-contract-check`.

## Rotation Checklist

1. Modify `toolchain/seed/src/main.vit`.
2. Regenerate or edit `toolchain/seed/vittec0.seed`.
3. Run `make seed-rotation-report` to inspect the current artifact hash and
   version.
4. Run `make seed-manifest-update` to rewrite `toolchain/seed/manifest.txt`
   from the audited artifact and source `VERSION_TEXT`.
5. Run `make bootstrap-native-snapshots`.
6. Run `make bootstrap-verify`.
7. Run `make seed-contract-check`.

## Local Checks

```sh
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
- `compiler-test-suite-bridge-gate`
- `driver-native-json-surface-gate`

These are complementary to `make bootstrap-native-snapshots`. The snapshot suite
locks the emitted stage artifacts and diagnostics; the integration gates verify
that the current driver still exposes the expected native JSON envelopes, that
the compiler entry builds without a bridge sidecar, and that the compiler test
suite bridge remains scoped to compiler test sources.

## Local Parallelism

CI jobs run in isolated workspaces, so their bootstrap artifacts do not collide.
In one local worktree, bootstrap targets that rebuild the stage chain are not
parallel-safe because they rewrite shared `bin/vittec0`, `bin/vittec1`,
`bin/vittec`, and `bin/vitte` artifacts.

The single-workspace runner rule is:

- `make bootstrap-native-snapshots` installs the seed and mutates
  `bin/vittec0`.
- `make bootstrap-verify` rebuilds `bin/vittec1`, `bin/vittec`, and
  `bin/vitte`.
- `make bootstrap-posix-smoke` depends on `bootstrap-all`, so it also rebuilds
  the stage artifacts before running POSIX checks.
- `make bootstrap-native-contract` composes those flows and should be treated as
  an exclusive bootstrap runner in a local worktree.

`tools/bootstrap_native_snapshots.sh` and `tools/bootstrap_posix_smoke.sh` use
per-run temporary directories under `target/` and clean them on exit, but the
shared `bin/vittec*` artifacts are intentionally not isolated. Avoid running
bootstrap targets concurrently in the same worktree.

## Source Coverage

The readable seed and stage sources should exercise the current
bootstrap-native forms they rely on:

- `toolchain/seed/src/main.vit` must include named string constants, named int
  constants, `proc main(args: list[string]) -> int`, and a named constant return.
- `toolchain/stage2/src/main.vit` must include named `VERSION_TEXT` and
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
- Any generated binary behavior change must update `emission.sha256.must`.

Coverage can be inspected with:

```sh
tools/bootstrap_native_fixture_matrix.sh
```
