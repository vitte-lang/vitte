# Bootstrap Seed

The sole trust-root strategy is a native stage0 signed for each supported
OS/architecture tuple. `toolchain/bootstrap/stage0-manifest.json` pins its
artifact, detached signature, public key, SHA-256 and native format. Verification
is offline and fail-closed before the binary is executed.

The accepted chain is `stage0 -> bootstrap compiler -> stage1 -> stage2 ->
release -> bin/vitte`. Stage1 and stage2 must have byte parity, a repeated build
must reproduce the hashes, and every compiler artifact is scanned for shell,
payload and self-copy markers.

```sh
make seed-verify
make bootstrap-seed
make bootstrap-source-of-truth
```
