# Bootstrap Contracts

This page is the index for the bootstrap-native contract surface.

- Seed trust chain: `docs/bootstrap_seed.md`
- Native IR contract: `docs/bootstrap_native_ir.md`
- Bootstrap-native snapshots: `tests/bootstrap_native/README.md`
- Local full contract target: `make bootstrap-native-contract`
- Local fast contract target: `make bootstrap-native-fast-contract`
- CI job: `bootstrap-native-contract`

The bootstrap contract is intentionally split across readable docs, checked seed
artifacts, deterministic snapshots, and visible CI steps. A change that affects
the seed parser, generated shell backend, command behavior, or stage1/stage2
source must carry the matching snapshots in the same change.

## PR Checklist

Before publishing a bootstrap-native change:

- Run `make bootstrap-native-contract`.
- Run `make docs-paths-check`.
- Run `make bootstrap-contracts-index-check`.
- Run `tools/bootstrap_native_fixture_matrix.sh` and confirm the touched surface
  is covered by snapshots.
- If `toolchain/seed/vittec0.seed` changed, confirm
  `toolchain/seed/manifest.txt` changed with the new SHA-256.
- If generated shell behavior changed, confirm the relevant shell snapshot under
  `tests/bootstrap_native/` and `emission.sha256.must` snapshots changed
  together.
- If parser or diagnostic behavior changed, confirm at least one `.ir.must` or
  `.err.must` snapshot changed with it.
