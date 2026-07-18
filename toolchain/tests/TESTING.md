# Bootstrap Testing

The active bootstrap test flow is:

```text
toolchain/seed/vittec0.seed
  -> bin/vittec0
  -> manifest/configuration verification
  -> deterministic IR and executable emission
  -> compiler-entry and native user-program gates
```

Run the focused checks with:

```sh
make seed-verify
toolchain/scripts/bootstrap/verify.sh
toolchain/test_bootstrap_reproducibility.sh
make bootstrap-native-snapshots
make bootstrap-vitte-hard-gate
```

The reproducibility test installs the same pinned seed twice and compares the
seed, manifest, installed artifact, and version hashes. Native snapshots pin
the current seed command surface and real compiler entry.

For self-hosting status, run:

```sh
python3 tools/selfhost_completion_audit.py
python3 tools/selfhost_completion_audit.py --strict-complete
```

The normal audit succeeds when the generation chain executes and reports
transition state. Strict completion additionally requires byte parity and no
transition payload; it is intentionally failing while those gaps remain.
