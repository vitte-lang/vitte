# Language Core Guarantees

## Purpose

This document states the minimum guarantees Vitte currently makes about the protected language core.

It is intentionally narrower than the whole language surface.

Related documents:

- `docs/LANGUAGE_CORE.md`
- `docs/LANGUAGE_CORE_COMPATIBILITY.md`
- `docs/LANGUAGE_CORE_CHECKLIST.md`
- `docs/LANGUAGE_CORE_TEST_PLAN.md`
- `docs/LANGUAGE_CORE_SURFACE.md`
- `docs/GENERIC_CALL_SYNTAX.md`

## Current Contract

Vitte does not guarantee broad language stability yet.

Vitte does currently guarantee that the protected core is:

- covered by a dedicated regression gate
- documented as a bounded surface
- separated from `experimental` and `internal` behavior
- expected to change more slowly and more intentionally than the rest of the project

## What The Core Guarantees Today

### 1. Protected Surface

The current protected surface includes the language center listed in `docs/LANGUAGE_CORE_SURFACE.md`.

In practice, this means the project is actively protecting:

- core declarations: `proc`, `entry`, `let`, `give`
- basic control flow: `if`, `otherwise`, `loop`, `for`, `match`, `break`, `continue`
- import and visibility boundaries around `use`, `pull`, and `share`
- explicit low-level boundaries such as `unsafe` and `asm`

### 2. Protected Diagnostics

The core gate currently protects a focused set of frontend diagnostics.

These include, at minimum:

- missing return on typed procedures
- unknown type
- unknown identifier
- invalid signed-to-unsigned cast
- strict import and strict module diagnostics
- canonical entry attachment diagnostics
- explicit share-list diagnostics
- duplicate share-declaration diagnostics
- duplicate import-binding diagnostics
- import/local binding collision diagnostics
- duplicate local-declaration diagnostics
- declared share lists narrow the module export surface when present
- duplicate public glob aliases of the same module do not raise false re-export conflicts
- module-alias member/type access outside the exported surface fails deterministically
- symbol imports that target non-exported names fail deterministically
- unknown generic bases and empty generic argument lists fail deterministically
- missing qualified type members on known local types fail deterministically
- ambiguous import path
- re-export conflict on public glob exports
- missing stdlib module
- experimental/internal import denial

The exact protected set is versioned in:

- `tests/diag_snapshots/core_diagnostic_codes.txt`

### 3. Conservative Semantics Are Intentional

Some current guarantees are conservative by design.

Today this means:

- typed-procedure return analysis is intentionally conservative
- `if` and `match` fallback behavior is protected
- bare `loop` and `for` do not prove total return on their own
- nominal generic usage is covered, but fine-grained generic diagnostics are not yet part of the protected core contract
- explicit generic constructors and explicit generic proc calls may be implemented and tested outside the core gate, but they are still experimental until promoted

Conservative behavior should not be widened silently.

### 4. Regression Gate Is The Enforcement Point

The minimum executable contract for the language core is:

```bash
make core-language-gate
```

For release-oriented validation of the protected core, use:

```bash
make core-release-gate
```

## What The Core Does Not Guarantee Yet

The following remain outside the current guarantee boundary unless explicitly promoted:

- advanced generic semantics
- explicit generic proc-call syntax and its ambiguity rules
- detailed generic diagnostics behavior
- macro ergonomics and expansion policy
- broad stdlib-driven surface stability
- backend/codegen guarantees beyond the protected frontend checks
- all currently implemented syntax outside the documented core surface

If something is not listed in the protected surface, treat it as `experimental`.

## Maturity Vocabulary

Use these words consistently:

- `stable`: protected by the core gate or another explicit compatibility policy
- `experimental`: implemented or documented, but not yet part of the protected contract
- `internal`: not a public contract

These labels are descriptive, not aspirational.

## Release Rule

No release should present the language as broadly stable unless the visible documentation still matches:

- the protected surface
- the protected diagnostics
- the current conservative semantic limits

If docs imply more than the gate protects, the docs are wrong.

## Summary

The current Vitte promise is not "the language is stable".

The current promise is:

- the core is bounded
- the core is executable as a regression contract
- the boundary between `stable`, `experimental`, and `internal` is explicit
