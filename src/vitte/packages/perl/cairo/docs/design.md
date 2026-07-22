# Cairo Design

Cairo is the structured data package for Vitte Perl tooling.

The design keeps small text codecs compatible with the original scalar, record,
and list functions while adding typed helpers for tables, schemas, document
encoding, deterministic JSON, and structured errors.

Each submodule has a matching focused test file. `t/basic.t` remains the full
integration surface, while `t/scalar.t`, `t/record.t`, `t/list.t`, `t/table.t`,
`t/schema.t`, `t/codec.t`, and `t/errors.t` isolate regressions by domain.
