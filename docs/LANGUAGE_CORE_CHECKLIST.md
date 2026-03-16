# Language Core Checklist

## Purpose

This checklist turns the core stability policy into day-to-day repository work.

Primary policy:

- `docs/LANGUAGE_CORE.md`

Grammar reference and gates:

- `book/grammar/README.md`
- `src/vitte/grammar/vitte.ebnf`
- `tests/grammar`

## How To Use This Checklist

Use this file before:

- adding a keyword
- changing grammar shape
- changing a core semantic rule
- changing beginner-facing examples
- expanding stable stdlib usage of a feature

If a change touches the language core, the goal is to leave every applicable section below in a clear state:

- `done`
- `not applicable`
- `blocked with explanation`

## A. Core Syntax

Confirm the change affects one of the intended core areas only:

- `proc`
- `entry`
- `let`
- blocks
- calls
- assignment
- simple operators
- `give`
- `if` / `else`
- `loop`
- `for`
- `match`
- `break`
- `continue`
- `use`
- import or visibility rules
- `unsafe`
- `asm`
- `ffi`

Checks:

- The change is clearly inside the core, or clearly marked non-core.
- No overlapping syntax is introduced without removing an ambiguity first.
- No new keyword is added unless an existing form cannot express the use case cleanly.
- Beginner readability is preserved or improved.

## B. Grammar Source Of Truth

Checks:

- The grammar source of truth was updated only in `src/vitte/grammar/vitte.ebnf` when grammar changed.
- Generated grammar artifacts were synced when needed.
- `book/grammar/grammar-changelog.md` records the change when the accepted surface changed.
- The change is categorized correctly as breaking, non-breaking, or diagnostic-only.

Useful files:

- `src/vitte/grammar/vitte.ebnf`
- `book/grammar/grammar-surface.ebnf`
- `book/grammar/vitte.ebnf`
- `book/grammar/grammar-changelog.md`

## C. Valid And Invalid Corpus

Checks:

- At least one canonical valid example exists for the changed core construct.
- At least one canonical invalid example exists for the changed core construct.
- New examples are minimal and isolate one behavior.
- Existing valid programs did not become invalid by accident.

Useful files:

- `tests/grammar/valid`
- `tests/grammar/invalid`

## D. Diagnostics

Checks:

- Parse or frontend diagnostics remain deterministic.
- Expected diagnostics were updated when wording or codes intentionally changed.
- Snapshots were refreshed only for intended changes.
- The error remains actionable for a contributor reading it cold.

Useful files:

- `book/grammar/diagnostics/expected`
- `tests/grammar/snapshots`
- `src/compiler/frontend/README.md`

## E. Semantics

Checks:

- The change does not silently widen or narrow core meaning without documentation.
- Return behavior, control-flow behavior, and module behavior remain explicit.
- Ambiguous semantics are resolved in docs before being used as a stable foundation.
- Experimental semantics are not presented as stable examples.

Minimum expectation:

- one canonical nominal example
- one edge-case example
- one failure example when relevant

For return/control-flow changes:

- document whether the rule is intentionally conservative
- add nested cases before widening inference
- do not treat `loop` or `for` as guaranteed-return constructs without explicit policy and coverage

## F. Documentation

Checks:

- Beginner-facing examples in `README.md` still compile conceptually and remain representative.
- Relevant book chapters still match the current language surface.
- Core policy docs stay aligned with actual grammar and diagnostics.
- Any experimental feature remains labeled as experimental in docs.

Useful files:

- `README.md`
- `docs/LANGUAGE_CORE.md`
- `book/chapters/27-grammaire.md`
- `book/chapters/41-versionnement-migration-code.md`
- `book/grammar/README.md`

## G. Standard Library Impact

Checks:

- The stdlib is not being used to force an unstable language feature into de facto stability.
- Stable packages avoid depending on unresolved experimental syntax where possible.
- New package surface is deferred if the core change is still unsettled.

Decision rule:

- if the core is unclear, reduce package expansion first

## H. Migration Risk

Checks:

- The change has an explicit migration story if it breaks existing source.
- The migration scope is understood across examples, tests, and packages.
- Temporary compatibility layers are used only when they reduce real migration risk.
- Breaking changes are not hidden inside unrelated refactors.

Useful files:

- `book/chapters/41-versionnement-migration-code.md`
- `book/packages-migration-plan.md`
- `book/packages-migration-map.md`

## I. Release Readiness For A Core Change

A core change is not ready to merge until the applicable statements below are true:

- Grammar source, generated artifacts, and changelog are aligned.
- Valid and invalid grammar corpus cover the change.
- Diagnostics are updated intentionally.
- Docs and beginner examples stay coherent.
- Migration impact is understood.
- The change makes the language simpler, clearer, or more robust.

## J. Fast Review Questions

Before approving a core change, ask:

1. Does this simplify the center of the language?
2. Can a new contributor learn it from one canonical example?
3. Can we lock it with a valid test, an invalid test, and a diagnostic snapshot?
4. Does it avoid pushing churn into the stdlib and docs?
5. If we had to keep this syntax for a long time, would we be comfortable doing so?

If the answer to one of these is `no`, the feature likely belongs in `experimental`, not in the stable core.
