# Async Foundation (171)

Date: May 22, 2026

## Objective

Prepare compiler/runtime integration for async execution.

## Implemented Foundation

- HIR lowering inspects items/statements/expressions for async/await markers.
- Async intent is propagated at item level through lowered metadata markers.
- Baseline misuse diagnostic:
  - `HIRASYNC0001`: `await` outside async procedure.

## Contracts

- `await` is only valid when procedure is async-marked.
- Async markers survive lowering so MIR and later passes can branch behavior.
- Non-async code paths remain unaffected.

## Diagnostics Contract

- Misuse is reported at HIR lowering stage as semantic error.
- Diagnostic includes remediation note:
  - mark procedure async, or
  - remove await.

## Follow-up Work

- Dedicated async type (`Future[T]`) propagation in typeck.
- Await lowering tied to explicit effect model.
- Async borrow and lifetime diagnostics beyond baseline misuse detection.
