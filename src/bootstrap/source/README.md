# Source manager

This module owns the compiler source registry. Each registered source gets a
non-zero `vitte_source_id_t`; zero is reserved for spans created outside a
manager. The manager stores the display name, source text, and a precomputed
line-start index.

Use `vitte_source_manager_add_copy` when the input buffer may disappear after
parsing, or `vitte_source_manager_add_borrowed` when the caller guarantees the
buffer lifetime. Offsets are byte offsets and spans use a half-open
`[start_offset, end_offset)` range. Lines and columns are one-based display
locations derived from offsets through `vitte_source_manager_location`.

The driver owns one manager per compilation run and exposes it through
`vitte_driver_sources`. Source ids flow through module, lexer token, AST span,
and diagnostic objects. Legacy line/column fields remain cached temporarily
for renderer compatibility; source id and offsets are canonical.

## Thread safety and failure cleanup

The manager has no process-global registry or mutable static cache. Give each
concurrent compilation its own `vitte_source_context_t`; concurrent read-only
lookups are valid while the context is unchanged, while add/reset/destroy
operations require exclusive ownership. Every registration is transactional:
name, path, source bytes and line offsets are released when a later allocation,
UTF-8 validation or limit check fails. `vitte_source_reset` returns the manager
to a reusable empty generation, and `vitte_source_validate` can be called
after every failure boundary.
