# Coroutine Lowering (172 Foundation)

Date: May 22, 2026

## Objective

Transform async-style control flow into a state-machine-oriented MIR skeleton.

## Implemented Foundation

- HIR->MIR pass detects await-like expressions in statements.
- For await-bearing statements, lowering inserts coroutine control-flow skeleton:
  - suspend block
  - resume block
  - continuation edge to normal control flow

This establishes a minimal resumable execution model in MIR.

## Control-Flow Contract

- Suspend and resume are represented as explicit MIR terminator behavior.
- Resume target is deterministic and linked to continuation block.
- Existing non-await statements keep standard lowering behavior.

## Safety and Correctness Scope

Foundation phase includes:
- explicit suspension/resumption edges
- predictable continuation points

Deferred to advanced phase:
- full generator frame layout
- precise captured-local mapping
- drop-order and cancellation guarantees
- panic/unwind semantics through suspended frames

## Backend Impact

- Backend sees explicit coroutine-like CFG edges in MIR.
- No full ABI/runtime coroutine frame guarantee is claimed yet; this is a structural lowering base.
