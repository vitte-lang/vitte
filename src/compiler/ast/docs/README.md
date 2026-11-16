# AST port status

`src/compiler/ast/ast.vitte` implements the arena-backed AST contract in native
Vitte: node creation and ownership, ordered children, attributes, flags,
semantic attachments, literal payloads, traversal, statistics, and deep
validation. Its arena is externally owned, reset invalidates nodes by advancing
the generation, and node storage is never individually freed.

The native parser in `src/compiler/parser/parser.vitte` keeps its own node
table. It exposes `vitte_parser_lower_to_ast` to copy a successful parse into
this arena-backed AST, while the current native driver passes the parser tree
directly to semantic analysis. The two representations have distinct
ownership and node layouts; the original C AST implementation remains under
`src/bootstrap/ast/` as the bootstrap reference.
