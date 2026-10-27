# Vitte C17 Backend

The C17 backend emits deterministic C17 from validated compiler IR. It depends
only on compiler layers and does not include or depend on `runtime/*`.

Core pieces:
- `options` owns stable backend configuration: includes, indentation, newline mode, output metadata, and debug comments.
- `writer` emits to a fixed buffer or `FILE *`, tracks bytes and lines, applies indentation, and reports overflow or I/O failures through `vitte_error_t`.
- `naming` sanitizes Vitte identifiers into valid C identifiers, rejects empty names, avoids C17 reserved words, and validates the small compiler operator set.
- `translation_unit` emits the C prelude and tracks include/declaration/function counts.
- `module` maps IR globals/functions/instructions to C17 text.
- `program` and `backend` provide the public emission surface for IR-to-buffer/file.

Supported compiler mapping:
- `int` -> `int`
- `i64` -> `int64_t`
- `usize` and unsigned compiler integer aliases -> `size_t`
- `bool` -> `bool`
- `string`/`str` -> `const char *`
- IR globals: `static const`
- IR control flow: labels plus `goto` / conditional `goto`
- selected builtin IR calls: `print`, `println`, `eprint`, `eprintln`, `panic`, `assert`, `len`

Limitations:
- Parameter modifiers such as `ref` and `mut` are lowered before this backend and arrive as plain IR value parameters.
- Unknown types, unsupported operators, and unsupported IR instructions are rejected with backend errors.
- String literals are escaped for C output, including quotes, backslashes, control characters, and non-printable bytes.
