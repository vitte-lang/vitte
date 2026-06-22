# diagnostics

Path: `src/vitte/compiler/infrastructure/diagnostics`

## Purpose

Shared compiler diagnostics infrastructure.

This directory is the compatibility layer used by compiler-facing infrastructure
code that still depends on `compiler/infrastructure/*`, while converging on the
richer diagnostics contract already used by the canonical `src/vitte/compiler/diagnostics`
stack.

## Contract

- One diagnostic has:
  - a stable code
  - a phase
  - a severity
  - a primary span
  - optional labels
  - optional notes
  - optional helps
  - optional suggestions
- `diagnostic.vit` owns the common data model and report counters.
- `labels.vit` owns label-specific constructors.
- `suggestions.vit` owns structured fixes and machine-applicable edits.
- `emitter.vit` renders the shared model for human-oriented text output.

## Invariants

- Use spans, not bare line/column pairs, as the stable location contract.
- Suggestions must stay structured even when rendered as plain text.
- A fatal diagnostic is always an error; not every error is fatal.
- Reports are valid only when they contain no error and no fatal diagnostic.
- New phase-specific producers should add labels and suggestions before adding
  more free-form prose.

## Near-term direction

- Route older frontend/session emitters toward this shared model.
- Keep text and JSON rendering derived from the same underlying diagnostic data.
- Prefer adding one new structured field over inventing another ad-hoc string.
