# Vitte Toolchain

The toolchain is Vitte-in-Vitte only.

- Canonical compiler source: `src/vitte/compiler/main.vit`
- Stage 0 compiler: `target/release/vitte`
- Stage 1 compiler: `target/stage1/vitte`
- Stage 2 compiler: `target/stage2/vitte`
- Release compiler: `target/release/vitte`

Removed source roots and artifacts are blocked by `make vitte-in-vitte-gate`.

Blocking gates:

- `make seed-free-release-gate`
- `make vitte-in-vitte-gate`
- `make build`
