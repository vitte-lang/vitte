# Bootstrap Seed

The sole trust-root strategy is a native stage0 signed for each supported
OS/architecture tuple. `toolchain/bootstrap/stage0-manifest.json` pins its
artifact, detached signature, public key, SHA-256 and native format. Verification
is offline and fail-closed before the binary is executed.

The accepted chain is `stage0 -> bootstrap compiler -> stage1 -> stage2 ->
release -> bin/vitte`. A repeated build must reproduce each stage hash, and
every compiler artifact is scanned for shell, payload, and self-copy markers,
including `vitte_stage0_clone_self`.

Cryptographic validity is necessary but not sufficient. A correctly signed
artifact is rejected when it contains a binary-copy implementation. The
currently committed macOS arm64 artifact has a valid RSA-SHA256 signature and
pinned digest, but is rejected by the strengthened trust policy because it
contains the retired clone-self path. It must be rotated by the stage0 key owner
after the source-built candidate is runtime-complete.

```sh
make bootstrap-trust-root
make bootstrap-trust-tests
make compiler-source-sensitivity-gate
make stage0-rotation-readiness
```

`stage0-rotation-readiness` is the fail-closed handoff to the signing-key owner.
It requires the 971-module candidate to be source-sensitive, copy-free and able
to run `--version`, `check`, `build`, and the resulting program. The private key
is never read or created by readiness checks. Signing and manifest replacement
may happen only after this target reports `ready`.
