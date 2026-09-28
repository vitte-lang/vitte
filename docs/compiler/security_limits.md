# Compiler Input Hardening Limits

This page defines mandatory limits and clean-failure behavior for untrusted inputs.

## Mandatory limits

- max file size
- max token size
- max AST depth
- max expression depth
- max import depth
- max modules count
- max diagnostics budget
- max symbol budget
- max parser recursion budget
- max macro expansion budget
- optional compile-time budget (future hook)

## Path validation

- reject path traversal (`..`)
- reject non-normalized path forms
- reject absolute paths outside project policy
- reject invalid/binary-like path payloads
- enforce sysroot/project boundary policy for source/import paths

## Clean failures

- return user-facing diagnostics on limit violations
- return `InvalidInput` for hostile input
- reserve ICE/InternalError for compiler bugs only
- never crash on malformed input

## Smoke checks

Run:

```sh
make security-input-limits-smoke
```

Covers:

- huge source payload
- huge token/string payload
- deep import path
- extreme expression nesting
- path traversal attempt
- invalid unicode payload
