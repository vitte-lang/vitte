# Bootstrap C17 Backend

The C17 backend emits deterministic C17 from the bootstrap AST and bootstrap
IR. It depends only on bootstrap layers and does not include or depend on
`runtime/*`.

Core pieces:
- `options` owns stable backend configuration: includes, indentation, newline mode, output metadata, and debug comments.
- `writer` emits to a fixed buffer or `FILE *`, tracks bytes and lines, applies indentation, and reports overflow or I/O failures through `vitte_error_t`.
- `naming` sanitizes Vitte identifiers into valid C identifiers, rejects empty names, avoids C17 reserved words, and validates the small bootstrap operator set.
- `translation_unit` emits the C prelude and tracks include/declaration/function counts.
- `module` maps AST declarations or IR globals/functions/instructions to C17 text.
- `program` and `backend` provide the public emission surface for AST-to-buffer/file and IR-to-buffer/file.

Supported bootstrap mapping:
- `int` -> `int`
- `i64` -> `int64_t`
- `usize` and unsigned bootstrap integer aliases -> `size_t`
- `bool` -> `bool`
- `string`/`str` -> `const char *`
- module declarations: procedure and const declarations
- statements: block, `give`/return, let, if/else
- expressions: integer, string, identifier, binary expression, call expression
- IR globals: `static const`
- IR control flow: labels plus `goto` / conditional `goto`
- selected builtin IR calls: `print`, `println`, `eprint`, `eprintln`, `panic`, `assert`, `len`

Limitations:
- Parameter modifiers such as `ref` and `mut` are currently parsed but lowered as plain C value parameters.
- Unknown types, unsupported operators, unsupported AST nodes, and unsupported IR instructions are rejected with backend errors.
- String literals are escaped for C output, including quotes, backslashes, control characters, and non-printable bytes.
