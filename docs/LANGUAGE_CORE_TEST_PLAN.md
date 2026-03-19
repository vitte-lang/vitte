# Language Core Test Plan

## Purpose

This document defines how Vitte validates the stabilized center of the language.

Related documents:

- `docs/LANGUAGE_CORE.md`
- `docs/LANGUAGE_CORE_CHECKLIST.md`
- `docs/GENERIC_CALL_SYNTAX.md`
- `tests/grammar/core_manifest.txt`
- `tests/diag_snapshots/core_semantic_manifest.txt`
- `tests/core_semantic_success_manifest.txt`
- `tests/diag_snapshots/core_diagnostic_codes.txt`

## Current Objective

The objective is not to prove the whole language stable.
The objective is to make core drift visible and expensive.

Today, the core gate focuses first on parser-level contracts:

- valid core programs stay valid
- invalid top-level misuse stays invalid
- core diagnostics remain deterministic

It also includes a first semantic layer for the language center:

- core identifier and type-resolution diagnostics
- core generic-base and missing-generic-argument diagnostics
- core signed/unsigned cast diagnostics
- strict canonical type-name enforcement in `--strict-types`
- import boundary rules for `use`
- `pull` / `share` parser and public-export surface checks
- `entry` semantic attachment checks
- strict-import diagnostics for canonical and explicit imports
- strict-modules diagnostics for glob imports
- module visibility boundaries
- missing stdlib module diagnostics

## Core Gate

Primary command:

```bash
make core-language-gate
```

This gate currently runs:

1. grammar sync check
2. core grammar corpus validation via a dedicated manifest
3. focused semantic diagnostics snapshots for the language center

Supporting commands:

```bash
make core-language-test
make core-language-test-update
make core-semantic-success
make core-semantic-snapshots
make update-diagnostics-ftl
make diagnostics-ftl-check
```

## Corpus Layout

Manifest:

- `tests/grammar/core_manifest.txt`
- `tests/diag_snapshots/core_semantic_manifest.txt`
- `tests/core_semantic_success_manifest.txt`

The manifest intentionally points to a small subset of `tests/grammar/valid` and `tests/grammar/invalid`.
This subset exists to protect the language center from silent drift.

Current core coverage includes:

- `proc`
- `entry`
- `use`
- `pull`
- `share`
- `let`
- `give`
- `if`
- `loop`
- `for`
- `break`
- `continue`
- `match`
- `unsafe`
- `asm`

Current `entry` grammar coverage includes:

- canonical `entry ... at ... { ... }` success
- missing `at` failure
- missing module path failure
- missing block opener failure
- block-level `pull` misuse failure
- block-level `share` misuse failure

Current `entry` semantic coverage includes:

- canonical entry module path success
- relative entry module path failure
- duplicate entry name failure

Current `pull` / `share` grammar coverage includes:

- canonical `pull path as alias` success
- canonical `share all` success
- missing `share` target failure

Current semantic core coverage includes:

- unknown identifier diagnostics
- unknown type diagnostics
- unknown generic base diagnostics
- generic type without arguments diagnostics
- qualified type member miss diagnostics
- invalid signed-to-unsigned cast diagnostics
- strict-types rejects compatibility aliases in the protected frontend core
- nominal generic pick/type application succeeds
- strict-import alias and canonical-path checks
- strict-modules glob import denial
- explicit share-list validation for known public symbols
- duplicate share declarations are rejected deterministically
- duplicate imported binding names are rejected deterministically
- import bindings that collide with local declarations are rejected deterministically
- duplicate local declaration names are rejected deterministically
- declared share lists narrow exported module-index surface
- symbol imports and `use module.{a, b}` stay inside the declared exported surface
- module-alias member and type access stay inside the declared exported surface
- ambiguous import path diagnostics for module loading
- re-export conflict diagnostics for public exports
- stdlib module lookup failures
- internal module import denial
- experimental module import denial
- missing return value on typed procedures

Current typed-procedure return coverage includes:

- `if` without fallback fails
- `if/otherwise` with full return paths succeeds
- nested `if/otherwise` return paths succeed
- nested `if` without inner fallback fails
- `match` without fallback fails
- `match` with explicit fallback succeeds
- `match` branch with nested `if/otherwise` succeeds
- `match` branch with nested missing fallback fails
- bare `loop` does not count as a guaranteed return path
- bare `for` does not count as a guaranteed return path
- loop bodies followed by explicit fallback return succeed
- procedure without explicit return type may end without error

Current import-boundary success coverage includes:

- local name/type resolution succeeds in a minimal typed proc
- non-negative unsigned cast resolves successfully at the protected frontend stage
- nominal generic `pick` usage with concrete type arguments succeeds
- canonical type names succeed under `--strict-types`
- local `pull ... as alias` with `share all` succeeds in a minimal module
- explicit `share ping, pong` succeeds for local declarations
- explicit `share dep` succeeds for a pulled alias facade
- explicit `share a_mod` can suppress unrelated glob re-export conflicts outside the declared public surface
- `share all` accepts duplicate glob aliases when they refer to the same underlying module
- `use facade.{dep, dep}` fails with a duplicate import-binding diagnostic in a local module fixture
- `use facade.{dep}` succeeds when `dep` is actually exported
- `use facade/dep as dep_mod` can coexist with a local `proc dep()` when names stay distinct
- `use facade/dep as dep` fails when a local declaration also uses `dep`
- local declarations with distinct names succeed
- `proc ping()` plus `const ping` fails with a duplicate local-declaration diagnostic
- `use facade as facade_mod` allows `facade_mod.dep()` when `dep` is directly exported by the module
- `use facade as facade_mod` allows `facade_mod.Public` when `Public` is directly exported by the module
- `facade_mod.hidden()` fails with a module-alias member-export diagnostic
- `facade_mod.Hidden` fails with a module-alias member-export diagnostic
- `Public.Missing` fails with a qualified-type-member diagnostic when `Public` is known locally
- `Option.Missing(...)` fails with the same qualified-type-member diagnostic when `Option` is a known local `pick`
- `x(1)` fails with a dedicated non-callable-expression diagnostic when `x` is a known local value
- `--strict-imports` accepts an explicitly aliased local import when the alias is used
- `--strict-modules` accepts an explicitly aliased local import when the alias is used
- `--allow-experimental` admits explicit experimental imports

Current generic boundary note:

- nominal generic type application is covered
- explicit generic constructors like `Public[T](...)` and `facade_mod.Public[T](...)` are implemented and snapshot-tested in IR, but remain outside the protected core gate
- explicit generic proc calls like `id[T](...)` are implemented for unambiguous type arguments and snapshot-tested in IR, but remain outside the protected core gate
- ambiguous forms such as `id[i](...)` intentionally stay on index-then-call parsing
- `E1003` and `E1004` are covered in the protected core gate
- finer generic diagnostics beyond the frontend resolve layer are not yet part of the protected core gate
- `--allow-internal` admits explicit internal imports

## Inclusion Rules

Add a file to the core manifest only if at least one of these is true:

1. The construct is part of the language core policy.
2. The construct appears in beginner-facing documentation.
3. Breaking the construct would cause broad migration across examples or stable packages.

Do not add a feature just because it exists.
The core manifest should stay selective.

## Snapshot Rules

For each core fixture:

- valid files must have an empty diagnostics snapshot
- invalid files must have an expected diagnostics contract
- snapshots must change only when the diagnostic behavior intentionally changes

Relevant paths:

- `tests/grammar/snapshots`
- `book/grammar/diagnostics/expected`

## Maturity Labels

Use these labels consistently in docs and review:

- `stable`: protected by the core gate and expected to change rarely
- `experimental`: implemented, but not protected as part of the language center
- `internal`: not part of the public language contract

Current interpretation:

- constructs listed in the core manifest are on the path to `stable`
- constructs outside that manifest default to `experimental` unless another document states otherwise

## Recommended Next Expansions

After this parser and resolve-level gate is reliable, the next useful layers are:

1. semantic fixtures for return/control-flow invariants
2. a small set of compile-success core examples
3. migration notes for intentional breaking changes
4. a sharper split between stable and experimental type-system features

## Review Standard

A core language change is not complete until:

- the manifest still represents the intended core
- the fixtures demonstrate the changed behavior minimally
- the snapshots were reviewed, not merely refreshed
- the docs still describe the same language center

## Summary

The core gate is intentionally narrow.
Its job is to lock the center first, not to absorb the entire language surface.
