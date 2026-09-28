# Contributing To The Vitte Stdlib

Every new stdlib module must include:

- a manifest entry;
- a validation entry in `tools/stdlib/run_checks.py`;
- at least one smoke or contract test;
- at least one example in `src/vitte/stdlib/examples`;
- generated API documentation.

Do not add direct OS access in `std`; route it through `platform` or compiler
platform shims. Do not add allocation dependencies in `core`.

## Review Checklist

Before review, run `make stdlib-gate`. A stdlib patch must include:

- an API entry and stability class for each public module;
- a dedicated test or fuzz contract for changed behavior;
- an example under `src/vitte/stdlib/examples`;
- memory invariant coverage for alloc changes;
- no public panic unless the diagnostic behavior is documented.

## Fuzzing

Fuzz contracts live under `src/vitte/stdlib/tests/fuzz` and cover UTF-8, URL,
CSV, JSON, and path normalization. New parsers must add a fuzz entry before they
are treated as stable.
