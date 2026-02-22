# ADR 0003: Scheduler Baseline Model

## Status

Proposed

## Context

Scheduler behavior impacts almost every kernel path, so baseline invariants must be explicit before optimization.

## Decision

Start with a baseline scheduler model:

- Runnable queue invariants
- Deterministic wake/sleep transitions
- Explicit context-switch boundaries

Add policy variants (realtime/deadline) after baseline tests are stable.

## Consequences

- Easier debugging and reproducibility.
- Lower risk of hidden starvation/fairness regressions in early stages.
