# bootstrap/src/scope

The scope layer manages lexical bindings for semantic analysis.

## Contract

- No dependency on `runtime/*`.
- One implicit global frame exists after initialization.
- Frame depth and binding storage are bounded.
- Current-scope duplicate definitions are rejected.
- Lookup walks outward from the innermost scope.
- Errors use `bootstrap/src/api/error.h`.

## Scope Model

- global frame
- nested block frames
- function boundary frames

Bindings point at symbols stored in `bootstrap/src/symbol`.

## Current Limits

- no namespace partition between type and value scopes
- no deferred pop markers or checkpoints
- no closure capture model yet
