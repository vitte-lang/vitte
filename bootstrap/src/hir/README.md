# bootstrap/src/hir

HIR is the high-level intermediate representation used by the bootstrap
compiler after AST construction and before lower-level IR or backend emission.

## Contract

- HIR nodes are allocated from `vitte_arena_t`.
- No direct `malloc` or `free` is used by this layer.
- Errors use `bootstrap/src/api/error.h`.
- The layer has no dependency on `runtime/*`.
- Node ids are stable, non-zero, and monotonic inside one `vitte_hir_t`.
- Lists store `first`, `last`, and `count`.
- Nodes are stable after construction except list `next` links.
- Validation and traversal are deterministic and depth-limited.

## Node Kinds

The bootstrap HIR currently models:

- module
- function
- block
- return statement
- let statement
- if statement
- integer literal
- string literal
- variable
- binary expression
- call expression
- type name
- error node

## Ownership

`vitte_hir_init` borrows an external arena. `vitte_hir_init_owned` creates and
owns an arena internally. `vitte_hir_destroy` releases only the owned arena case.

AST source pointers are optional backreferences. HIR does not own AST nodes.

## Builders

`vitte_hir_builder_t` provides constructors for all node kinds and helpers for:

- adding functions to a module
- adding statements to a block
- adding arguments to a call

Builders return `NULL` on invalid input, arena failure, invalid kind, or id
overflow. The owning `vitte_hir_t` records the last error.

## Lowering

`vitte_hir_lower_ast` lowers the current bootstrap AST model:

- AST module to HIR module
- proc declaration to function
- block statement to block
- `give` statement to return
- let statement to let
- if statement to if
- integer and string literals
- identifier to variable
- binary expression
- call expression
- type name
- error node

Const declarations are represented as HIR error nodes until the global-decl
model is expanded.

## Validation

`vitte_hir_validate` checks:

- initialized HIR
- root module exists
- valid node kind and non-zero id
- coherent list counts
- function name/body
- block statement lists
- binary operands
- call callee
- type names
- maximum recursion depth

## Traversal And Debug

`vitte_hir_visit` performs pre-order traversal. A callback returning `false`
stops traversal at that node. `vitte_hir_dump` writes a compact tree to `FILE *`
without depending on the printer module.
