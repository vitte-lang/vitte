# bootstrap/src/parser

The parser turns bootstrap Vitte source into the arena-backed AST used by the
rest of the C17 bootstrap pipeline.

## Contract

- No dependency on `runtime/*`.
- Input tokens come from `bootstrap/src/lexer`.
- Nodes are allocated through `bootstrap/src/ast` and its arena.
- Syntax diagnostics go through `bootstrap/src/diagnostic`.
- Errors use `bootstrap/src/api/error.h`.
- Parsing is deterministic and bounded by `max_depth`.

## Entry Points

- `vitte_parser_init`
- `vitte_parser_init_module`
- `vitte_parser_parse_module`
- `vitte_parser_parse_decl`
- `vitte_parser_parse_stmt`
- `vitte_parser_parse_expr`

The normal bootstrap path is:

1. initialize an empty AST
2. initialize the parser on source text or a loaded module
3. parse a module root
4. validate the AST
5. hand the AST to diagnostics, codegen, and backend stages

## Supported Grammar

Current bootstrap grammar:

- module: zero or more top-level declarations
- declaration:
  - `proc name() -> type { ... }`
  - `proc name() { ... }`
  - `const name: type = expr`
  - `const name = expr`
- statement:
  - block `{ ... }`
  - `give expr`
  - `let name: type`
  - `let name: type = expr`
  - `if expr stmt`
  - `if expr stmt else stmt`
- expression:
  - integer literal
  - string literal
  - identifier
  - grouped expression `(expr)`
  - call `callee(arg, ...)`
  - unary `+expr` and `-expr`
  - binary `* / + - < <= > >= == !=`

## Bootstrap Limits

- procedure parameters are rejected for now
- `let` requires an explicit type in the bootstrap parser
- `const` without a type relies on simple literal/operator inference
- there are no expression statements yet

## Recovery

When `recover_errors` is enabled, the parser keeps scanning after a syntax
error and synchronizes on:

- `;`
- `}`
- next declaration keyword at top level
- next statement opener inside blocks

This keeps diagnostics useful without building malformed list structure into
the AST.

## Module Integration

`vitte_parser_init_module` parses a `vitte_module_t` that already owns loaded
source text. On success, the parser attaches the AST to the module and advances
its state to `PARSED`.

## Driver Integration

The bootstrap driver now uses this parser instead of the previous synthetic AST
builder. `check`, `emit-c`, and `build` all pass through the same parse stage.
