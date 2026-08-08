# Bootstrap C17 Backend

The C17 backend emits deterministic C17 from the bootstrap AST. It depends only on `api`, `arena`, and `ast`; it does not include or depend on `runtime/*`.

Core pieces:
- `options` owns stable backend configuration: includes, indentation, newline mode, output metadata, and debug comments.
- `writer` emits to a fixed buffer or `FILE *`, tracks bytes and lines, applies indentation, and reports overflow or I/O failures through `vitte_error_t`.
- `naming` sanitizes Vitte identifiers into valid C identifiers, rejects empty names, avoids C17 reserved words, and validates the small bootstrap operator set.
- `translation_unit` emits the C prelude and tracks include/declaration/function counts.
- `module` maps AST declarations, statements, expressions, and type references to C17 text.
- `program` and `backend` provide the public emission surface for AST-to-buffer and AST-to-file.

Supported bootstrap mapping:
- `int` -> `int`
- `i64` -> `int64_t`
- `bool` -> `bool`
- `string`/`str` -> `const char *`
- module declarations: procedure and const declarations
- statements: block, `give`/return, let, if/else
- expressions: integer, string, identifier, binary expression, call expression

Limitations:
- Function parameters are not modeled by the current bootstrap AST and are emitted as `(void)`.
- Unknown types, unsupported operators, and unsupported AST nodes are rejected with backend errors.
- String literals are escaped for C output, including quotes, backslashes, control characters, and non-printable bytes.
