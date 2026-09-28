# Reproducibility

Seed reproducibility compares repeated installations, manifest state, versions,
IR, and generated native fixtures:

```sh
toolchain/test_bootstrap_reproducibility.sh
toolchain/scripts/bootstrap/verify.sh
make bootstrap-native-snapshots
```

Compiler-generation reproducibility is a stricter contract:

```sh
python3 tools/selfhost_completion_audit.py --require-parity
```

The strict command remains failing while successive generated compiler hashes
differ.
