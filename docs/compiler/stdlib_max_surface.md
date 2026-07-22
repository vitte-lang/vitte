# Vitte Max Stdlib Surface

This document defines the max standard-library surface required before Vitte can
claim a broad systems-language standard library for version 0.1.0.

## Scope

The max surface is implemented in Vitte source syntax under `src/vitte/stdlib`.
Runtime-dependent behavior is exposed as compiler/runtime intrinsic contracts;
it is not implemented as hidden foreign wrappers.

Required families:

- algorithms: ordering, sorting, search, bounds, rotation, uniqueness
- numeric: limits, accumulation, reduction, inner product, prefix sums, gcd/lcm
- memory: layout, address, copy, move, fill, equality, uninitialized allocation
- iterators: iterator traits, ranges, traversal, distance, collection, predicates
- utility: pair, exchange, move/forward contracts, const view, unreachable
- functional: invoke, identity, compose, bind, predicate adapters, comparators
- tuple: fixed tuple forms, accessors, tie, concatenation
- optional: optional value state, construction, access, reset, emplacement
- variant: two-way variant, index, holds, get, visit
- expected: typed success/error channel, mapping, chaining
- span: non-owning typed contiguous view, slicing, byte view
- string-view: non-owning text view, slicing, find, prefix/suffix checks

## Gate

`make stdlib-max-gate` is blocking. It verifies:

- every required module exists
- every required symbol fragment is present in the Vitte source
- every required module has at least 30 source lines
- every required module has at least 10 public procedures
- every required module has at least 8 native procedures whose implementation is not a
  direct `compiler_*` intrinsic handoff
- every module is an official stdlib entrypoint
- every module has public example and test coverage
- `./bin/vitte check` passes for every max-surface module
- the max-surface contract test checks successfully
- reports are written to `target/reports/stdlib_max_gate.json` and
  `target/reports/stdlib_max_gate.md`

`make stdlib-gate` depends on this max gate, so release validation cannot pass
with an incomplete max stdlib surface.
