# Bootstrap AST

The AST layer owns syntax nodes through `vitte_arena_t`. Nodes are zero-initialized and allocated from the arena; `vitte_ast_destroy` only destroys the arena when the AST was initialized with `vitte_ast_init_owned`.

Invariants:
- Node kinds are always in `vitte_ast_node_kind_t`.
- Nodes are immutable after construction except `next` and list fields.
- Source spans are optional; valid spans keep stable source name, offsets, lines, and columns.
- Lists maintain `first`, `last`, and `count`.
- Errors use `bootstrap/src/api/error.h`.
- The AST layer does not depend on `runtime/*`.

Node kinds:
- module, proc decl, const decl
- block, give, let, if
- integer literal, string literal, identifier
- binary expr, call expr, type name
- error node

Builders in `ast.h` construct each node shape and attach list members through explicit helpers such as `vitte_ast_module_add_decl`, `vitte_ast_block_add_stmt`, and `vitte_ast_call_add_arg`.

Validation checks root presence, node kind ranges, required procedure/body fields, coherent expression operands, call callees, and depth limits. Traversal is pre-order and stops when the callback returns false.
