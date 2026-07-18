# Toolchain Examples

## Install And Verify The Seed

```sh
make seed-verify
make bootstrap-seed
bin/vittec0 --version
```

## Check And Build A Program

```sh
bin/vittec0 check tests/golden/frontend/fixtures/hello_min.vit
bin/vittec0 build tests/golden/frontend/fixtures/hello_min.vit -o target/hello
target/hello
```

## Run The Hard Gate

```sh
make bootstrap-vitte-hard-gate
make bootstrap-verify
```

## Inspect Self-Hosting Progress

```sh
python3 tools/selfhost_completion_audit.py
python3 tools/selfhost_completion_audit.py --strict-complete
```

The normal command records transition state. The strict command requires byte
parity and no bridge payload.

## Rotate The Seed

Only for an intentional reviewed seed change:

```sh
make seed-rotation-report
make seed-manifest-update
VITTE_UPDATE_BOOTSTRAP_SHELL_SNAPSHOTS=1 make bootstrap-native-snapshots
make seed-contract-check
make bootstrap-native-snapshots
```
