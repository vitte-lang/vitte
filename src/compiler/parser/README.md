# Parser port status

`parser.vitte` describes the parser's public data model, and its 18 procedures
are still `export extern proc` bindings. Its token, span, node, enum, and parser
context layouts match `parser.h`. The active parser and complete grammar
remain in `src/bootstrap/parser/parser.c` and `parser.h`.

The native pipeline currently re-lexes through a namespaced bootstrap lexer,
checks its token count and fingerprint against the Vitte lexer, then calls the
bootstrap parser. This keeps the parser's C context private while the Vitte
parser and lexer use different storage layouts. The native port still needs to
implement token ownership, parser storage, diagnostics, recovery, grammar, and
validation in Vitte.

`ASTNodeKind` includes the bootstrap enum's optional type node.
`ASTNode.node_flags` retains the C field's position and width. `ASTNode` is
still parser-owned; it is not the separate arena-backed AST type in
`src/compiler/ast/ast.vitte`.

Attribute prefixes are retained in `Parser.attributes`. Each declaration node
stores a slice into that table, and each record points to token ranges for the
qualified name and its positional or named arguments. Use
`vitte_parser_node_attribute_count` and `vitte_parser_get_attribute` to read
the records, then resolve the token indices through the parser token stream.
