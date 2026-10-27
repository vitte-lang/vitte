# Vitte Max Stdlib Surface (Target)

This document describes the desired maximum standard-library surface. It is a scope list, not a claim that every module or validation gate is present today. Standard-library source files are rooted at `modules/`; the former `src/vitte/stdlib` tree is gone.

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
