# Bootstrap tests

The active bootstrap contract is:

```text
toolchain/seed/vittec0.seed
  -> bin/vittec0
  -> target/bootstrap/stage1/vittec1
  -> bin/vittec1
  -> target/bootstrap/stage2/vittec
  -> bin/vittec + bin/vitte
```

`toolchain/bootstrap-config.json` is the versioned contract. The checker validates stage order, producer edges, source versions, artifact kinds, installation paths, aliases and hashes.

## Commands

Static contract only:

```bash
toolchain/tests/bootstrap-tests.sh quick
```

Static contract and existing artifacts:

```bash
toolchain/tests/bootstrap-tests.sh full
```

Artifacts plus stage1/stage2 command parity:

```bash
toolchain/tests/bootstrap-tests.sh advanced
```

Rebuild the complete chain twice and compare every stage hash:

```bash
toolchain/test_bootstrap_reproducibility.sh
```

Run the strict bootstrap gate, including native snapshots:

```bash
make bootstrap-vitte-hard-gate
```

## Expected identities

```text
vittec0 stage0-vitte-seed 0.1.0
vittec1 stage1-vitte 0.1.0
vittec2 stage2-vitte 0.1.0
```

Stage2 is currently a machine executable with `bridge_policy` set to `transitional-allowed`. The test output reports that state explicitly. Full self-hosting requires rebuilding stage2 without the bridge and changing the policy to `forbidden`.
