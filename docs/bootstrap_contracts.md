# Bootstrap Contracts

This page is the index for the bootstrap-native contract surface.

## Overview

Bootstrap contracts are split across artifacts, readable specs, snapshots, and
gates so that a contributor can understand both the human intent and the exact
machine-checked surface.

| Contract area | Primary reference |
| --- | --- |
| Seed trust chain | `docs/bootstrap_seed.md` |
| Native IR format | `docs/bootstrap_native_ir.md` |
| Snapshot evidence | `tests/bootstrap_native/README.md` |
| Full local gate | `make bootstrap-native-contract` |
| Fast local gate | `make bootstrap-native-fast-contract` |

- Seed trust chain: `docs/bootstrap_seed.md`
- Native IR contract: `docs/bootstrap_native_ir.md`
- Bootstrap-native snapshots: `tests/bootstrap_native/README.md`
- Local full contract target: `make bootstrap-native-contract`
- Local fast contract target: `make bootstrap-native-fast-contract`
- CI job: `bootstrap-native-contract`

The bootstrap contract is intentionally split across readable docs, checked seed
artifacts, deterministic snapshots, and visible CI steps. A change that affects
the seed parser, generated shell backend, command behavior, or canonical
compiler entry must carry the matching snapshots in the same change.

## Responsibilities

- Keep bootstrap trust boundaries obvious during reviews.
- Point contributors to the right contract document for each bootstrap layer.
- Tie source edits to the exact gates that defend them.

## Invariants

- Seed, IR, generated shell behavior, and snapshots must move together.
- A change to the bootstrap surface without updated evidence is considered
  incomplete.
- The fast local gate may optimize execution time, but not reduce the contract
  coverage it claims to represent.

## Pipeline

The bootstrap-native path connects documentation to executable verification:

1. The seed and native IR specs describe the accepted surface.
2. Bootstrap sources and generated artifacts produce deterministic outputs.
3. Snapshot suites pin those outputs.
4. CI and local make targets enforce the contract continuously.

## Driver

Bootstrap is exposed through the driver surface as much as through compiler
internals, so driver behavior changes must be considered part of this contract.

## Data Flow

1. Review starts from the seed and IR docs.
2. Source edits update seed, stage, or emitted shell behavior.
3. Snapshots and contract targets confirm that the visible bootstrap output
   still matches the documented rules.

## Examples

```sh
make bootstrap-native-fast-contract
make bootstrap-native-contract
tools/bootstrap_native_fixture_matrix.sh
```

These commands are useful in combination: the fast target gives a quick signal,
the full contract target verifies the whole native chain, and the fixture matrix
shows whether the touched bootstrap form is already covered by a snapshot.

## PR Checklist

Before publishing a bootstrap-native change:

- Run `make bootstrap-native-contract`.
- Run `make docs-paths-check`.
- Run `make bootstrap-contracts-index-check`.
- Run `tools/bootstrap_native_fixture_matrix.sh` and confirm the touched surface
  is covered by snapshots.
- If `toolchain/seed/vittec0.seed` changed, confirm
  `toolchain/seed/manifest.txt` changed with the new SHA-256.
- If generated shell behavior changed, confirm the relevant `shell.*.must`
  snapshots under `tests/bootstrap_native/` changed with it.
- If parser or diagnostic behavior changed, confirm at least one `.ir.must` or
  `.err.must` snapshot changed with it.
