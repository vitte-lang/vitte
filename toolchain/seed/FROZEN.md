# Frozen Seed Policy

`toolchain/seed/vittec0.seed` is a historical bootstrap artifact.

It is kept only as a fallback bootstrap entry point for rebuilding an initial
stage0 in an empty environment. It is not the active source of compiler
development and must not be edited as part of normal compiler, stdlib, package,
installer, or release work.

Frozen files:

- `toolchain/seed/vittec0.seed`
- `toolchain/seed/src/main.vit`
- `toolchain/seed/manifest.txt`
- `toolchain/seed/frozen.json`

The normal compiler path must move through `src/vitte/compiler` and
`src/vitte/stdlib`. Any intentional seed rotation requires an explicit override
outside the normal CI path and a new reviewed bootstrap plan.
