# Legacy C17 Host Sources

The repository still carries a small host-language bootstrap implementation for
the retained C17 compatibility path. These files are not the current Vitte
compiler driver and must not be treated as runtime provenance evidence.

## Declared Roots

The following roots are explicitly owned by `make bootstrap-c17`:

- `bootstrap/include/runtime/`: C17 runtime support headers.
- `bootstrap/include/vitte/`: C17 compiler ABI and subsystem headers.
- `bootstrap/src/`: the legacy C17 bootstrap implementation, including lexer,
  parser, semantic analysis, IR, driver, and C17 code generation.
- `toolchain/sysroot/include/`: target sysroot headers passed to cross-toolchain
  and C17 compatibility builds.

These roots are reported by `tools/selfhost_audit.sh` as **C17 compatibility
sources**. They are excluded from the unclassified legacy-source failure, but
their use remains limited to the explicit `bootstrap-c17` targets.

## Audit Rule

Any C, C++, or header file outside the declared roots, the runtime bridge, and
the excluded editor fixtures is an unclassified host source and remains an
audit failure. The Vitte self-host chain is defined by
`src/vitte/compiler/main.vit` and `src/vitte/compiler/driver/compiler.vit`.

## Migration Rule

Do not add new files to these roots. Changes to the C17 compatibility path must
pass `make bootstrap-c17-smoke` and must not change the stage0/stage1/stage2
provenance chain. Once the C17 path is retired, archive these roots together and
remove their Makefile targets and this exception from the audit.
