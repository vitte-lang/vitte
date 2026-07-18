# Vitte Bootstrap Toolchain

The bootstrap has one trust root: `toolchain/seed/vittec0.seed`. Its reviewed
Vitte source is `toolchain/seed/src/main.vit`, and
`toolchain/seed/manifest.txt` pins the executable hash and version.

## Active Flow

```text
toolchain/seed/vittec0.seed
  -> scripts/seed/install_seed.sh
  -> bin/vittec0
  -> native bootstrap contracts and compiler-entry checks
  -> src/vitte/compiler/main.vit
```

There are no numbered bootstrap source stages. Generated compiler generations
used by the self-hosting audit are temporary artifacts and are never trust
roots or fallbacks.

## Entry Points

- `toolchain/bootstrap.sh`: user-facing bootstrap dispatcher.
- `scripts/seed/install_seed.sh`: verified seed installation.
- `tools/bootstrap_vitte_hard_gate.sh`: strict native bootstrap gate.
- `tools/check_bootstrap_stage_chain.py`: seed configuration and artifact
  verifier; the historical filename is retained for command compatibility.
- `src/vitte/compiler/main.vit`: canonical compiler source entry.

## Commands

```sh
make seed-verify
make bootstrap-seed
make bootstrap-source-of-truth
make bootstrap-native-snapshots
make bootstrap-vitte-hard-gate
make bootstrap-verify
```

`make bootstrap-all` delegates to the hard gate. None of these commands probes
or invokes an older compiler.

## Trust And Rotation

An intentional seed rotation updates the executable, manifest, and generated
shell snapshots in one reviewed change:

```sh
make seed-manifest-update
VITTE_UPDATE_BOOTSTRAP_SHELL_SNAPSHOTS=1 make bootstrap-native-snapshots
make seed-contract-check
make bootstrap-vitte-hard-gate
```

The readable seed source records intent, while the manifest-pinned executable
is the code that actually runs. `make seed-verify` checks their identity
boundary.

## Current Self-Hosting Status

`tools/selfhost_completion_audit.py` builds the real compiler entry with the
seed and asks that generated compiler to rebuild it. The chain executes, but
the generated shell payload still uses a transition bridge and successive
compiler generations are not byte-identical. Consequently full autonomous
self-hosting is not yet complete; `--strict-complete` remains expected to fail
until both conditions are removed.

## Layout

```text
toolchain/
├── bootstrap-config.json
├── bootstrap.sh
├── seed/
│   ├── manifest.txt
│   ├── src/main.vit
│   └── vittec0.seed
├── scripts/
├── src/
└── tests/
```

Runtime C bridge files under `src/vitte/compiler/backends/runtime_c` are an
explicit native runtime boundary, not a bootstrap trust fallback.
