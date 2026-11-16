# Native Vitte parser

`parser.vitte` implements the parser used by the native compiler pipeline. It
owns parser nodes, attributes, diagnostics, recovery, grammar recognition,
validation, fingerprints, and lowering into the arena-backed AST. The parser
borrows its input token array and token lexemes; it releases the storage it
allocates when reset or destroyed.

The bootstrap C parser remains available as a compatibility reference. The
native parser exposes the same parser data layout and `vitte_parser_run` routes
to `vitte_parser_run_native`. The driver lexes with the Vitte lexer, converts
the tokens to `ParserToken`, and parses them natively. Semantic analysis
consumes the parser-owned tree directly.

The Vitte API also exposes the parser and node helpers from the bootstrap
header: state and generation queries, span operations, node-kind categories,
flag checks, child access, and the attribute-count alias.

## Grammar coverage

`grammar/vitte.ebnf` is the canonical syntax inventory. The native parser
covers its declaration, type, statement, pattern, and expression productions:

- declarations, modifiers, attributes, exports, imports, generics, bounds,
  contracts, and nested declaration blocks
- procedure, pointer, reference, array, slice, optional, and path types
- blocks, bindings, assignment statements, control flow, match arms, `defer`,
  assertions, unsafe blocks, and assembly statements
- tuple, list, constructor, struct, literal, range, wildcard, and alternative
  patterns
- literals, paths, calls, indexing, member access, casts, unary and binary
  operators, generic form initializers, conditional expressions, and block
  expressions

The grammar describes syntax, not type checking or name resolution. Reserved
compatibility keywords that have no production in `vitte.ebnf` remain rejected
by the parser.

## Diagnostics and recovery

Parser diagnostics retain the error kind, source span, expected token, and
found token. Duplicate diagnostics are suppressed only when they describe the
same error at the same span, so a later independent error can still be
reported before synchronization completes. Recovery tracks nested delimiters,
recognizes `>>` as two generic closers, and synchronizes at list separators,
closing delimiters, statement or declaration boundaries, and the next `case`
or `else` match arm. Recovery stops after the configured token budget and
never consumes an unmatched closing delimiter belonging to an enclosing
production. At an unambiguous follow point, the parser treats a missing
semicolon or closing delimiter as inserted, records the diagnostic, and keeps
the completed node. Block expressions also synchronize after a malformed item
instead of discarding all later items. Parsing reports failure when it emitted
an error, while retaining the partial tree for diagnostics and inspection.

## Parser tree and arena AST

`Parser` stores a node table with stable numeric IDs. The optional
`vitte_parser_lower_to_ast` API copies a successful parse into the separate,
arena-backed AST defined by `src/compiler/ast/ast.vitte`. The current native
driver passes the parser tree directly to semantic analysis; it does not call
this conversion. The two node representations remain separate so parser
ownership and public AST storage can evolve independently.

## Frontend synchronization

`make test-grammar` checks keyword/operator coverage across the Vitte and C
lexers, verifies that the C lexer tokens are referenced by its parsers, and
runs the bootstrap parser's grammar fixtures. It does not prove
production-by-production equivalence with the native parser or editor
grammars. `src/tests/native_parser_parity_test.c` records additional parity
cases for the native parser and bootstrap reference.
