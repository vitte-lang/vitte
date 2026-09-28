# Bootstrap Seed

The sole trust-root strategy is a native stage0 signed for each supported
OS/architecture tuple. `toolchain/bootstrap/stage0-manifest.json` pins its
artifact, detached signature, public key, their SHA-256 digests and native
format. The verifier also pins the expected public-key digest in code; the
manifest therefore cannot authorize a replacement key. Verification is offline
and fail-closed before the binary is installed.

The accepted chain is `stage0 -> bootstrap compiler -> stage1 -> stage2 ->
release -> bin/vitte`. A repeated build must reproduce each stage hash, and
every compiler artifact is scanned for shell, payload, and self-copy markers,
including `vitte_stage0_clone_self`.

Cryptographic validity is necessary but not sufficient. A correctly signed
artifact is rejected when it contains a binary-copy implementation. The current
macOS arm64 trust root has a valid RSA-SHA256 signature, matches the pinned key,
and is free of the retired clone-self path. Installation uses a temporary file,
rechecks its digest, and atomically replaces the verified stage0 destination.

```sh
make bootstrap-trust-root
make bootstrap-trust-tests
make toolchain-manifests-gate
make compiler-source-sensitivity-gate
make stage0-rotation-readiness
```

`stage0-rotation-readiness` is the fail-closed handoff to the signing-key owner.
It requires the 971-module candidate to be source-sensitive, copy-free and able
to run `--version`, `check`, `build`, and the resulting program. The private key
is never read or created by readiness checks. Signing and manifest replacement
may happen only after this target reports `ready`.
