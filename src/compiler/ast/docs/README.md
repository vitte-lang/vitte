# AST port status

`src/compiler/ast/ast.vitte` implements the arena-backed AST contract in native
Vitte: node creation and ownership, ordered children, attributes, flags,
semantic attachments, literal payloads, traversal, statistics, and deep
validation. Its arena is externally owned, reset invalidates nodes by advancing
the generation, and node storage is never individually freed.

The parser still has a separate transitional node table in
`src/compiler/parser/parser.vitte`, whose functions remain C extern bindings.
That model is not yet wired to this AST store. Keep the parser migration
separate until its ownership and node representation are deliberately unified.
The original C implementation remains under `src/bootstrap/ast/` as the
bootstrap reference.
