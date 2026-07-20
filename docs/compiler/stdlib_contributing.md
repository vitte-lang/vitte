# Contributing To The Vitte Stdlib

Every new stdlib module must include:

- a manifest entry;
- a validation entry in `tools/stdlib/run_checks.py`;
- at least one smoke or contract test;
- at least one example in `src/vitte/stdlib/examples`;
- generated API documentation.

Do not add direct OS access in `std`; route it through `platform` or compiler
platform shims. Do not add allocation dependencies in `core`.
