# Language Core Surface

## Purpose

This document defines the current intended split between the protected language center and the rest of the surface.

Related documents:

- `docs/LANGUAGE_CORE.md`
- `docs/LANGUAGE_CORE_CHECKLIST.md`
- `docs/LANGUAGE_CORE_TEST_PLAN.md`
- `docs/GENERIC_CALL_SYNTAX.md`
- `docs/SYNTAX_STYLE_GUIDE.md`
- `docs/API_LIFECYCLE.md`

## Stable Core Path

The constructs below are the current protected center of the language.
They are the priority for regression tests, diagnostics stability, and documentation durability.

This document is intentionally narrower than `src/vitte/grammar/vitte.ebnf`.
The EBNF describes the implemented language surface.
This file defines the frozen core grammar surface that must remain stable unless the core policy is intentionally changed.

### Stable-Or-Being-Stabilized Core

- `proc`
- `entry`
- `use`
- `pull`
- `share`
- `let`
- `give`
- blocks `{ ... }`
- simple calls
- assignment
- `if` / `else` / `otherwise`
- `loop`
- `for`
- `break`
- `continue`
- `match` with explicit fallback
- `unsafe` as an explicit boundary
- `asm` as an explicit boundary

## Canonical Core Grammar Surface

The frozen core is defined by canonical source forms, not by broad feature families.
If a construct is implemented but does not appear below, it is outside the protected core.

### Canonical Top-Level Forms

Only these top-level forms are in the protected core surface:

- `proc name(...) { ... }`
- `proc name(...) -> Type { ... }`
- `entry name at module/path { ... }`
- `use package/path`
- `use package/path as alias`
- `pull module/path`
- `pull module/path as alias`
- `share all`
- `share a, b, c`

Not part of the frozen top-level core surface:

- `space`
- `const`
- global `let`
- `make`
- `type`
- `form`
- `trait`
- `pick`
- `macro`
- alternate legacy declaration forms

### Canonical Statement Forms

Only these statement forms are in the protected core surface:

- `let name = expr`
- `let name: Type = expr`
- `give expr`
- `if cond { ... }`
- `if cond { ... } else { ... }`
- `if cond { ... } otherwise { ... }`
- `loop { ... }`
- `while cond { ... }`
- `for item in expr { ... }`
- `break`
- `continue`
- `match expr { case Pattern { ... } otherwise { ... } }`
- `unsafe { ... }`
- `asm("...")`
- expression statements based on simple calls and assignment

Not part of the frozen statement core surface:

- `make`
- `set`
- `emit`
- `return`
- `select`
- `when`
- `when ... is ...`
- statement forms that exist only as parser convenience or experimental surface

### Canonical Expression Surface

The protected expression core is intentionally small:

- identifiers
- literals: `bool`, `int`, `string`, list literals
- grouped expressions `(expr)`
- simple calls: `name(...)` and `module.name(...)`
- assignment with `=`
- simple operator expressions
- nominal type names in annotations and return types

Explicitly outside the protected expression core:

- `if` expressions with explicit fallback
- explicit generic proc calls such as `id[T](...)`
- explicit generic constructor calls outside nominal type application
- parser heuristics that choose between generic-call parsing and index-then-call parsing
- postfix surface beyond simple calls that still relies on unstable behavior
- advanced type expressions beyond the documented core names

### Canonical Module And Visibility Surface

The frozen module core is the following:

- `entry` uses `entry name at module/path { ... }`
- `entry` module paths stay canonical
- `pull` and `use` stay top-level only
- `share` stays top-level only
- one module declares at most one `share` surface
- explicit `share` lists refer only to real local or aliased public symbols

Explicitly excluded from the protected module core:

- convenience import syntax beyond the canonical forms above
- unstable glob or grouped-import ergonomics beyond current deterministic diagnostics
- any import or export shape that requires parser heuristics to stay readable

### Core Semantic Invariants

- typed procedures must not fall off the end without returning a value
- typed-procedure return analysis is conservative for `loop` and `for`
- entry names must stay unique within one module
- entry module paths must stay canonical
- modules must not declare multiple `share` surfaces
- imported binding names must stay unique within one module
- imported binding names must not collide with local declaration names in one module
- local declaration names must stay unique within one module
- explicit `share` lists must reference real local or aliased public symbols
- declared `share` lists define the exported surface when present
- duplicate glob aliases of the same underlying module must not create false public export conflicts
- module-alias member and type access must stay inside the declared exported surface
- symbol imports must stay inside the declared exported surface of a module
- core generic-base and missing-type-argument diagnostics must stay deterministic
- import/export surface diagnostics for ambiguous paths and re-export conflicts stay deterministic
- internal modules stay private outside their owner namespace
- experimental modules stay opt-in
- missing stdlib modules fail with deterministic diagnostics

## Experimental Surface

The constructs below should be treated as experimental until explicitly promoted:

- macro expansion features
- advanced generics behavior
- explicit generic calls outside nominal type application, including `id[T](...)`
- parser heuristics for explicit generic calls on expression heads
- alternate or convenience syntax that overlaps existing core forms
- package-specific DSLs
- features that are implemented but not covered by the core gate
- semantics that still move frequently in docs or examples

## Internal Surface

The following are not public language contracts:

- `*/internal/*` module paths
- compiler implementation details
- backend lowering details
- unstable package facades marked internal by policy

## Current Exclusions From The Stable Core

These areas are intentionally not part of the protected center yet:

- advanced generics design
- explicit generic proc-call syntax, even when implemented for unambiguous cases
- fine-grained generic diagnostics (`E1003` / `E1004` / `E1006`) beyond nominal generic-type usage
- macro ergonomics and expansion policy
- package-specific DSLs
- broad stdlib-driven surface growth
- additional keywords that do not unlock a blocked use case
- syntax sugar that competes with existing readable forms
- legacy declaration syntaxes kept for compatibility or migration
- forms present in full EBNF but absent from `tests/grammar/core_manifest.txt`

## Decision Rule

If a feature is not clearly listed in the canonical core grammar surface, treat it as `experimental` unless another policy document says otherwise.
If a form is accepted by `src/vitte/grammar/vitte.ebnf` but absent from this file, do not treat that acceptance as a frozen public contract.
