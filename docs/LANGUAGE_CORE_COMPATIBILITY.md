# Language Core Compatibility

## Purpose

This document explains what may break in the protected language core during the experimental phase, and what must not break silently.

Related documents:

- `docs/LANGUAGE_CORE.md`
- `docs/LANGUAGE_CORE_GUARANTEES.md`
- `docs/LANGUAGE_CORE_SURFACE.md`
- `docs/LANGUAGE_CORE_TEST_PLAN.md`

## Current Rule

Vitte is still experimental.

That means core changes are still possible.

But for the protected language core, breakage must now be:

- explicit
- justified
- documented
- covered by updated tests and snapshots

## What Must Not Break Silently

The following must not drift without an intentional compatibility decision:

- protected core syntax accepted by the core manifests
- protected semantic invariants enforced by the core gate
- protected diagnostic codes listed in `tests/diag_snapshots/core_diagnostic_codes.txt`
- documented `stable` / `experimental` / `internal` boundaries

If one of these changes, the change must include docs and gate updates.

## What May Still Change

The following may still change during the experimental phase:

- wording outside the protected diagnostic set
- semantics outside the protected core surface
- advanced generics behavior
- macro ergonomics
- backend and codegen details outside explicit contract documents
- broad package behavior not covered by the core gate

These should still be documented when user-visible, but they are not yet part of the protected compatibility promise.

## Breaking Change Rule For The Core

A protected-core breaking change is acceptable only if all of the following are true:

1. The change removes ambiguity, unsoundness, or long-term maintenance cost.
2. The old behavior was documented as experimental or otherwise clearly unstable, or the migration benefit is stronger than the churn introduced.
3. The PR updates the relevant policy/guarantee docs.
4. The core gate is updated to reflect the new intended behavior.
5. A migration note is added when existing examples or packages are affected.

If one of these is missing, the change is not ready.

## Migration Note Minimum

When a protected-core change is breaking, include:

- what changed
- who is affected
- before/after example
- whether a temporary compatibility path exists
- when the old path stops being supported

Keep it short and concrete.

## Compatibility Levels

Use this scale when discussing core changes:

- `non-breaking`: same accepted core behavior, or additive-only
- `diagnostic-only`: behavior same, but codes/wording/notes changed intentionally
- `breaking`: accepted syntax, semantics, or protected contract changed

Protected-core diagnostic-only changes still require snapshot review.

## Release Expectation

Before release-facing work, run:

```bash
make core-release-gate
```

If release-facing docs imply a stronger promise than this gate enforces, the docs must be corrected first.

## Summary

The protected core is still allowed to evolve.

What changed recently is the standard of evidence:

- no silent breakage
- no undocumented breakage
- no contract drift without tests and migration notes
