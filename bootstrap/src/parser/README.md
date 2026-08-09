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

- module:
  - optional `space module/path`
  - zero or more `use` imports:
    - simple path `use math/core`
    - alias `use math::core as core`
    - glob `use math::*`
    - groups `use math::{add, sub as minus, ops::{mul}}`
  - zero or more top-level declarations
- declaration:
  - `export *`
  - `export name`
  - `export name as alias`
  - `export { name, other as alias }`
  - `export proc name(param: type, ...) -> type { ... }`
  - `export proc name(param: type, ...) { ... }`
  - `export const name: type = expr`
  - `export const name = expr`
  - `proc name(param: type, ...) -> type { ... }`
  - `proc name(param: type, ...) { ... }`
  - `const name: type = expr`
  - `const name = expr`
- statement:
  - block `{ ... }`
  - `give expr`
  - `let name: type`
  - `let name: type = expr`
  - `expr;`
  - `if expr stmt`
  - `if expr stmt else stmt`
- expression:
  - integer literal
  - string literal
  - identifier and qualified identifier `a::b`
  - grouped expression `(expr)`
  - call `callee(arg, ...)`
  - unary `+expr` `-expr` `!expr` `not expr`
  - binary `* / % + - << >> & ^ | < <= > >= == != and or && ||`
  - qualified type names `a::b`

## Bootstrap Limits

- import paths are normalized to dotted module names in AST/module tracking
- direct `use a::b` is locked as a module-path import
- symbol imports must use groups like `use a::{b}`
- when `use a::b` fails as a module import but `a` exports `b`, the driver emits a dedicated diagnostic pointing to `use a::{b}`
- standalone bootstrap export clauses only target local top-level items
- bootstrap re-exports such as `export mod::{x}` and `export mod as alias` are explicitly rejected with dedicated diagnostics
- `let` still requires an explicit type
- `const` without a type relies on simple literal/operator inference
- generics, where clauses, attributes, visibility, forms, picks, traits, impls,
  generalized re-export declarations, and patterns are still outside the bootstrap parser

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
source text. On success, the parser:

- attaches the AST to the module
- normalizes `space foo/bar` to module name `foo.bar`
- registers `use` imports into both the AST and `vitte_module_t`
- advances module state to `PARSED`

## Driver Integration

The bootstrap driver now uses this parser instead of the previous synthetic AST
builder. `check`, `emit-c`, and `build` all pass through the same parse stage.
