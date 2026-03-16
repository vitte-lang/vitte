# Language Core Surface

## Purpose

This document defines the current intended split between the protected language center and the rest of the surface.

Related documents:

- `docs/LANGUAGE_CORE.md`
- `docs/LANGUAGE_CORE_CHECKLIST.md`
- `docs/LANGUAGE_CORE_TEST_PLAN.md`
- `docs/GENERIC_CALL_SYNTAX.md`
- `docs/API_LIFECYCLE.md`

## Stable Core Path

The constructs below are the current protected center of the language.
They are the priority for regression tests, diagnostics stability, and documentation durability.

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
- broad stdlib-driven surface growth
- additional keywords that do not unlock a blocked use case
- syntax sugar that competes with existing readable forms

## Decision Rule

If a feature is not clearly listed in the stable core path, treat it as `experimental` unless another policy document says otherwise.
