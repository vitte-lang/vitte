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
