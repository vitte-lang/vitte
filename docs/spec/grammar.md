# Vitte Grammar and Parser Coverage

The normative language grammar is [`grammar/vitte.ebnf`](../../grammar/vitte.ebnf).
Published EBNF and Pest copies are generated from their canonical sources.

The grammar describes the Vitte language surface; it is not a claim that every
compiler route parses, type-checks, or emits every production. The trusted
stage0 seed has its own narrower contract in
[`selfhost_core_subset.md`](../compiler/selfhost_core_subset.md).

The active Vitte parser accepts a smaller subset, and parser acceptance
does not guarantee semantic or backend support. The current implementation
matrix and remaining parity gates are maintained in
[`frontend_native_port.md`](../bootstrap/frontend_native_port.md). In
particular, `for`, `match`, aggregate layouts, generics, traits, macros, and
async code must be assessed per compiler phase rather than inferred from their
presence in the normative grammar.

