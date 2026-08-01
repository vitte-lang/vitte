# Toolchain

The compiler source of truth is `src/vitte/compiler`. Vitte 0.1.0 bootstraps
from one explicit trust-root strategy: a native stage0 signed for each supported
OS/architecture tuple. The artifact, detached signature, public key, SHA-256 and
native format are pinned by `toolchain/bootstrap/stage0-manifest.json`.

No C prototype, shell compiler, embedded payload, network download or release
output may be used as an implicit stage0. The verified chain is:

`signed stage0 -> bootstrap compiler -> stage1 -> stage2 -> release -> bin/vitte`

Run `make bootstrap-all` for the chain and `make bootstrap-native-contract` for
the clean-checkout/offline contract. See `toolchain/bootstrap/README.md`.
