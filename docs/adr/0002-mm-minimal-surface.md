# ADR 0002: Minimal MM Surface First

## Status

Proposed

## Context

Memory subsystem complexity grows quickly and destabilizes adjacent modules when APIs are not constrained early.

## Decision

Prioritize a minimal memory API surface first:

- Allocation
- Mapping
- Error/failure signaling

Delay advanced features until baseline contracts are stable.

## Consequences

- Early implementation stays auditable.
- Refactor cost is reduced when adding paging/allocator variants later.
