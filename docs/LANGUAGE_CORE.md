# Language Core Policy

## Purpose

Vitte should stabilize a small language core before expanding the standard library or adding new surface syntax.

The goal is simple:

- keep the language readable
- keep compiler behavior predictable
- make documentation durable
- reduce migration cost while the project is still experimental

This document defines that core, what is currently inside it, and how changes must be evaluated.

Operational checklist:

- `docs/LANGUAGE_CORE_CHECKLIST.md`
- `docs/LANGUAGE_CORE_COMPATIBILITY.md`
- `docs/LANGUAGE_CORE_TEST_PLAN.md`
- `docs/LANGUAGE_CORE_SURFACE.md`

## Why This Policy Exists

If the language core changes while public packages and examples expand quickly, the project pays the same cost many times:

- more code to migrate
- more documentation to rewrite
- more tests and snapshots to refresh
- less confidence in what "stable" means

Vitte already has a broad package surface and a large documentation set.
That makes core discipline more valuable than adding new concepts.

## Definition Of The Language Core

The language core is the smallest set of syntax and semantics that must become boring, teachable, and strongly regression-tested.

The core is split into five areas:

1. basic declarations
2. control flow
3. core types
4. modules and visibility
5. low-level boundary features

Anything outside this set should default to `experimental` until proven stable.

## Core Scope

### 1. Basic Declarations

The core should keep these forms stable first:

- `proc`
- `entry`
- `let`
- block structure with `{ ... }`
- calls
- assignment
- simple operators
- explicit value return with `give`

### 2. Control Flow

The core should keep these forms stable before introducing alternatives:

- `if`
- `else`
- `loop`
- `for`
- `match`
- `break`
- `continue`

If any of these still have unresolved semantic ambiguity, the ambiguity must be resolved before new control-flow syntax is added.

Current stability note:

- typed-procedure return analysis is intentionally conservative
- `if` and `match` with explicit fallback are part of the protected semantic center
- bare `loop` and `for` do not currently prove a total return path on their own
- loop-based total-return analysis should not be widened silently without dedicated tests and docs

### 3. Core Types

The first stability target should be the smallest useful type set:

- `int`
- `bool`
- `string`
- explicitly declared procedure parameters and return types

Pointers, references, borrowed forms, slices, shape-like constructs, or advanced generic behavior should not be treated as stable unless their invariants are already documented and tested.

### 4. Modules And Visibility

These rules belong in the core because they shape every program:

- `use`
- `pull`
- `share`
- import resolution rules
- module path conventions
- export and visibility behavior
- entrypoint attachment rules

Current stability note:

- `entry` module paths are expected to be canonical
- duplicate public entry names in one module are rejected
- modules must declare at most one `share` surface
- imported binding names must stay unique within one module
- imported binding names must not collide with local declaration names in one module
- local declaration names must stay unique within one module
- explicit `share a, b` lists are validated against local declarations and explicit import aliases
- block-level misuse of top-level import/export forms should fail deterministically

If module behavior is unclear, stdlib growth should pause before introducing more public package surface.

### 5. Low-Level Boundary Features

These features are important, but they must have narrow, explicit contracts:

- `unsafe`
- `asm`
- native interop and `ffi`

The project should document:

- what guarantees still hold inside these boundaries
- what checks the compiler still performs
- what is intentionally left to the programmer

## Non-Core Surface

The following categories should be treated as non-core unless explicitly promoted:

- new keywords
- convenience syntax
- macro expansion features
- advanced generics features
- package-specific DSLs
- experimental type-system constructs
- implicit behavior that reduces readability

Non-core features should be clearly marked in docs as one of:

- `stable`
- `experimental`
- `internal`

## Change Policy

When a change touches the language core, the default question is not "can we add it?" but "does this make the core easier to teach, test, and preserve?"

A core change should meet all of these conditions:

1. It removes ambiguity or complexity, or unlocks an already-blocked use case.
2. It does not force wide migration across existing examples and packages without strong justification.
3. It can be covered by parser, semantic, and diagnostic regression tests.
4. It can be documented with one canonical example and one invalid example.
5. It improves or preserves readability.

If a proposal fails one of these checks, it should remain experimental or be deferred.

## Freeze Strategy

Until the core is judged stable enough, Vitte should follow these rules:

1. Do not add new syntax categories unless they remove a proven limitation.
2. Prefer clarifying semantics over introducing sugar.
3. Prefer fewer keywords over overlapping keywords.
4. Prefer explicit behavior over inference.
5. Prefer stable diagnostics over clever parser behavior.
6. Prefer conservative control-flow guarantees over unsound "smart" return inference.

This is a temporary bias, not a permanent ban on growth.
The point is to stabilize the center first.

## Exit Criteria For A Stable Core

The language core can be considered meaningfully stable when all of the following are true:

1. The canonical grammar for core constructs is frozen for a release window.
2. Core syntax has valid and invalid test coverage.
3. Core semantic rules have regression coverage.
4. Core diagnostics are snapshot-tested and intentionally worded.
5. Beginner examples in the README and book remain valid without churn across releases.
6. Package authors can build on the core without tracking constant syntax churn.

## Practical Priorities

The recommended order is:

1. freeze core syntax
2. freeze core semantics
3. freeze diagnostic expectations for the core
4. freeze module/import behavior
5. expand stable stdlib surface carefully

This order matters.
Growing packages before the language center is stable makes every later correction more expensive.

## What This Means For The Standard Library

The stdlib should follow the language core, not define it indirectly.

In practice:

- core packages should use only stabilized language behavior where possible
- experimental language features should not quietly become required by many packages
- package expansion should slow down if the language still has unresolved foundational questions

Large package inventories are useful, but they are not proof of language maturity.

## Recommended Repository Actions

To make this policy real, the project should maintain:

- one canonical core grammar reference
- one core language checklist
- one compatibility note for the protected core
- one test suite focused on valid and invalid core programs
- one compatibility note explaining what can break during the experimental phase
- one explicit label for docs or packages that are still experimental

## Summary

For Vitte, the main risk is not lack of ambition.
The visible risk is surface area expanding faster than the language center hardens.

The correct bias for the next phase is:

- freeze the core
- test the core
- document the core
- only then widen the stable surface
