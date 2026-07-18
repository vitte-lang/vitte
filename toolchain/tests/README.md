# Toolchain Tests

This directory owns seed integrity, reproducibility, generated compiler, and
self-host transition checks.

```sh
toolchain/tests/bootstrap-tests.sh quick
toolchain/tests/bootstrap-tests.sh full
toolchain/tests/bootstrap-tests.sh advanced
```

- `quick` validates the static one-seed configuration.
- `full` also verifies the installed seed artifact.
- `advanced` runs native snapshots and the generated compiler completion audit.

Tests must fail closed: no compatibility compiler or hidden fallback may replace
the manifest-pinned seed.
