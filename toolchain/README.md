# Toolchain

The compiler source of truth is `src/vitte/compiler`. Vitte 0.1.0 bootstraps
from one explicit trust-root strategy: a native stage0 signed for each supported
OS/architecture tuple. The artifact, detached signature, public key, SHA-256 and
native format are pinned by `toolchain/bootstrap/stage0-manifest.json`. The full
stage chain is registered in `toolchain/bootstrap/stages-manifest.json`.

No C prototype, shell compiler, embedded payload, network download or release
output may be used as an implicit stage0. The verified chain is:

`signed stage0 -> bootstrap compiler -> stage1 -> stage2 -> release -> bin/vitte`

The stage-specific contract roots are `toolchain/stage0/src`,
`toolchain/stage1/src` and `toolchain/stage2/src`. Their artifact manifests are
checked by `make toolchain-stage012-gate`.

For each stage, `config/` records the policy, `tests/` records executable
contract checks, and `reports/` is reserved for generated stage evidence.

Run `make bootstrap-all` for the chain and `make bootstrap-native-contract` for
the clean-checkout/offline contract. Run `make toolchain-stages-gate` to verify
the registered stage paths and `make bootstrap-max-gate` for the maximal
stage/locales/surface coverage. See `toolchain/bootstrap/README.md`.
