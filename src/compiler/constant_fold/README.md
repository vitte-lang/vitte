# Native constant values and folding

`constant.vitte` is the native Vitte counterpart of the primitive constant API
declared by `src/bootstrap/constant_fold/constant.h` and implemented by
`src/bootstrap/constant_fold/constant.c`.

## Native implementation status

The Vitte module implements the ABI records and enums, API name helpers,
classification, validation, bit width, zero/one checks, structural equality,
deterministic hashing, result queries, and all constant/result constructors.
String payloads are borrowed byte spans, matching the C API. The union payload
uses two machine words so it can hold the C string pointer and length; scalar
access copies the member bytes to preserve the C representation.

Unary and binary folding, casts, statistics, and the translation-unit anchor
are still declared `extern` and provided by the bootstrap C implementation.
The module does not yet evaluate AST nodes and is not currently imported by
the native frontend pipeline.

## Scope

This layer represents already-known primitive values: booleans, fixed-width
integers, floats, Unicode scalar values, borrowed strings, null, and poison.
It does not own AST/HIR/IR nodes, symbol resolution, diagnostics, or
arbitrary-precision numbers. Higher compiler layers remain responsible for
those concerns.

## ABI reference

Keep enum order, record field order, pointer constness, and integer widths in
sync with `src/bootstrap/constant_fold/constant.h`. The C declarations are the
public ABI reference while the native pipeline is staged.
