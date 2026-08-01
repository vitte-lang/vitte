# Vitte 0.1.0 bootstrap trust root

Vitte uses exactly one bootstrap strategy: a native stage0 compiler signed for
each supported OS/architecture tuple. There is no C bootstrap, shell seed,
payload bridge, self-copy dispatcher, network download, or fallback compiler.

The release authority publishes three tracked files per tuple:

1. `toolchain/bootstrap/stage0/<os>-<arch>/vitte[.exe]`;
2. its detached RSA/SHA-256 signature;
3. the release public key referenced by `stage0-manifest.json`.

The manifest pins the exact SHA-256, native format, signature and public key.
`tools/bootstrap_real/stage0_trust.py` verifies all of them before stage0 can be
executed. Verification is offline and fail-closed. A missing tuple, checksum,
signature, public key, OpenSSL executable, or unexpected binary format rejects
the bootstrap.

After trust verification, the only accepted chain is:

```text
signed stage0 -> bootstrap compiler -> stage1 -> stage2 -> release -> bin/vitte
```

Every generated compiler must be a native binary, contain the canonical
`run_cli_main_with_ice_boundary` entry marker, and contain none of the forbidden
payload/copy markers. Stage1 and stage2 must be byte-identical. A second clean
build must reproduce their hashes. `bin/vitte` is an installation copy of the
already verified release artifact; copying is forbidden as a compilation step,
not as the final installation operation.

Private release keys must never be committed. The project release owner signs
stage0 externally, commits only the binary, detached signature, public key and
manifest update, then runs the clean-checkout and offline gates.
