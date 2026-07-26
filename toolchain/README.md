# Toolchain

Vitte no longer ships or installs a bootstrap seed compiler.

The compiler source of truth is `src/vitte/compiler`. Stage and release
artifacts are produced by the Vitte compiler pipeline and checked by
`make vitte-in-vitte-gate`.

Legacy bootstrap-seed targets are compatibility aliases only. They execute the
Vitte-in-Vitte gate and must not create or require a seed artifact.
