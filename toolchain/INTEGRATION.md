# Toolchain Integration

## Build Systems

The Make and Steel entry points share the same seed contract:

```sh
make bootstrap-seed
make bootstrap-verify
steel run bootstrap_seed
steel run bootstrap_verify
```

`toolchain/bootstrap.sh quick`, `normal`, and `strict` delegate to the matching
Make gates. No integration command probes another bootstrap compiler.

## CI

`.github/workflows/bootstrap.yml` verifies the manifest, seed installation,
native snapshots, clean/offline checkout behavior, POSIX execution, and the hard
gate. Bootstrap-full matrix failures are blocking.

## Packaging And Installation

Scripts under `toolchain/scripts/{install,package}` consume built artifacts and
never establish trust. Seed identity is verified before bootstrap artifacts are
used. Release packages must include the manifest-pinned seed when they advertise
source bootstrap support.

## Reports

- `target/reports/bootstrap/hard_gate.json`
- `target/reports/bootstrap/hard_gate_native.json`
- `target/reports/repro_bootstrap/bootstrap_selfhost_repro.json`
- `target/selfhost_completion/selfhost_completion.json`

Generated reports describe one execution and are not source-of-truth inputs.
