# Self-Host Core Subset

This document defines the current **compiler self-host subset** for Vitte.

It is narrower than the broader language surface documented elsewhere. The goal
is not to describe every accepted syntax form, but to define the contract that
the trusted bootstrap path and stage entrypoints may rely on while the compiler
is still converging toward full self-hosting.

## Scope

This contract is enforced today for:

- `toolchain/seed/src/main.vit`
- `toolchain/stage1/src/main.vit`
- `toolchain/stage2/src/main.vit`
- `src/vitte/compiler/main.vit`

These files are the bootstrap and driver-facing trust anchors. The full
compiler tree still uses a broader language surface and is not yet required to
fit inside this subset.

## Required top-level shape

Every self-host subset module must use only these top-level constructions:

- `space <name>`
- `import <path>`
- `const <NAME>: <type> = <value>`
- `use <path> ...`
- `form <name> { ... }`
- `pick <name> { ... }`
- `proc <name>(...) -> <type> { ... }`
- `export *`

The bootstrap entry modules are also expected to expose:

- `const VERSION_TEXT: string = ...`
- `const BANNER_TEXT: string = ...`
- `proc version_text() -> string`
- `proc banner_text() -> string`
- `proc main(args: list[string]) -> int`

## Allowed declaration surface

The current self-host subset allows these declaration categories:

- named string constants
- named integer constants
- forms
- picks
- procedures
- module imports
- full-module export via `export *`

The following top-level constructs are **not** part of the current self-host
subset contract:

- `trait`
- `impl`
- `class`
- `union`
- `bits`
- `flags`
- `macro`
- `query`
- `compiler`
- `pass`
- `backend`
- `diagnostic`
- `test`
- `bench`

## Allowed statement surface

Inside procedures, the currently accepted self-host contract is intentionally
constrained but no longer wrapper-only. The trust root already uses:

- `let`
- `set`
- `if`
- `elif`
- `else`
- `while`
- `give`

The following statement or control-flow families are currently excluded from
the self-host contract:

- `with`
- `try`
- `defer`
- `unsafe`
- `asm`
- `emit`
- `panic`
- `select`
- `critical`

## Allowed type surface

The subset currently relies on these type forms:

- `int`
- `string`
- `bool`
- `list[string]`
- arrays such as `[string]`
- named compiler support types reached through `use`, such as
  `panic_boundary.PanicBoundaryState`

## Design rule

The self-host subset is a **contract-first** surface:

- keep stage entrypoints simple
- keep bootstrap wrappers explicit
- avoid using experimental syntax in trust-root files
- widen this subset only with matching gate updates

## Enforcement

The executable gate is:

```sh
make selfhost-subset-check
```

Bootstrap contract runners include it transitively through:

```sh
make bootstrap-native-contract
make bootstrap-native-fast-contract
```

## Expansion rule

Any intentional subset expansion must update all of:

- this file
- `docs/spec/grammar.md` when the parser-level core contract changes
- `tools/check_selfhost_subset.sh`
- bootstrap tests or snapshots when the trusted stage surface changes
