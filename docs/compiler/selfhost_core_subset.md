# Self-Host Core Subset

This document records only the constrained syntax subset used by the trusted stage0 source. It is not the implementation map for the active compiler: stage0 still supplies the executable and C host-toolchain boundary, while ordinary `compile` and `run` inputs use the linked Vitte frontend described in the [active pipeline](pipeline.md).

## Scope

The syntax subset applies to `toolchain/seed/src/main.vit`. The former compiler entry `src/vitte/compiler/main.vit` is retired; the staged native Vitte track does not yet have a complete replacement entry point.

## Required top-level shape

The seed subset uses these top-level constructions:

- `space <name>`
- `import <path>`
- `const <NAME>: <type> = <value>`
- `use <path> ...`
- `form <name> { ... }`
- `pick <name> { ... }`
- `proc <name>(...) -> <type> { ... }`
- `export *`

The seed entry exposes named version/banner constants and a `main(args: list[string]) -> int` procedure.

## Allowed declaration surface

The seed subset covers named string and integer constants, forms, picks, procedures, module imports, and full-module export through `export *`.

The following top-level constructs are outside this subset:

- `trait`, `impl`, `class`, `union`, `bits`, `flags`
- `macro`, `query`, `compiler`, `pass`, `backend`, `diagnostic`
- `test`, `bench`

## Allowed statement surface

The seed uses `let`, `set`, `if`, `elif`, `else`, `while`, and `return`.

The following statement or control-flow families are outside the subset:

- `with`, `try`, `defer`, `unsafe`, `asm`, `emit`, `panic`, `select`, `critical`

## Allowed type surface

The subset uses `int`, `string`, `bool`, `list[string]`, arrays such as `[string]`, and named support types imported with `use`.

## Design rule

Keep the trusted seed entry simple, keep bootstrap wrappers explicit, and avoid experimental syntax in trust-root files. Widening the subset requires matching seed contract updates.

For the current compiler paths, see [the compiler source map](../MODULE_REFERENCE.md) and [native frontend port notes](../bootstrap/frontend_native_port.md).
