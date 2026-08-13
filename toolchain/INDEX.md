# Vitte Toolchain

The toolchain is Vitte-in-Vitte only.

- Canonical compiler source: `src/vitte/compiler/main.vit`
- Signed Stage 0 compiler: `toolchain/bootstrap/stage0/macos-arm64/vitte`
- Trusted Stage 0 compiler: `target/bootstrap-real/stage0/vitte`
- Bootstrap compiler: `target/bootstrap-real/vitte`
- Stage 1 compiler: `target/stage1/vitte`
- Stage 2 compiler: `target/stage2/vitte`
- Release compiler: `target/release/vitte`
- Installed compiler: `bin/vitte`
- Installed compiler alias: `bin/vittec`

All canonical stage paths, hashes, formats and materialization commands are
registered in `toolchain/bootstrap/stages-manifest.json`.

Stage-specific source contracts and artifact manifests live in
`toolchain/stage0`, `toolchain/stage1` and `toolchain/stage2`. They are checked
by `make toolchain-stage012-gate`.

Each stage owns four deliberate surfaces: `src/` for compiler contracts,
`config/` for its policy, `tests/` for executable contract checks, and
`reports/` for stage-local report ownership. Stage0 is the signed trust root;
stage1 consumes the verified bootstrap compiler; stage2 consumes stage1 and
owns release-parity validation.

Removed source roots and artifacts are blocked by `make vitte-in-vitte-gate`.

Blocking gates:

- `make seed-free-release-gate`
- `make vitte-in-vitte-gate`
- `make toolchain-stages-gate`
- `make toolchain-stage012-gate`
- `make bootstrap-max-gate`
- `make build`
