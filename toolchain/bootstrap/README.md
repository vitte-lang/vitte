# Bootstrap Toolchain Stages

This directory contains the committed bootstrap trust root and the registered
stage chain used by `make bootstrap-all`.

- `stage0-manifest.json` pins the signed per-platform stage0 artifact.
- `stage0/macos-arm64/vitte` is the committed signed stage0 for macOS arm64.
- `stage0/macos-arm64/vitte.sig` is the detached RSA/SHA-256 signature.
- `stage0/stage0-public.pem` verifies the detached signature offline.
- `stages-manifest.json` records every canonical stage path, SHA-256, size,
  native format and materialization command.

The canonical chain is:

`signed_stage0 -> trusted_stage0 -> bootstrap_compiler -> stage1 -> stage2 -> release -> bin -> vittec`

Run:

- `make bootstrap-trust-root`
- `make bootstrap-all`
- `make toolchain-stages-gate`
- `make bootstrap-max-gate`
