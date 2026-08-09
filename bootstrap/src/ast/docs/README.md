# Bootstrap AST

The AST layer owns syntax nodes through `vitte_arena_t`. Nodes are zero-initialized and allocated from the arena; `vitte_ast_destroy` only destroys the arena when the AST was initialized with `vitte_ast_init_owned`.

Invariants:
- Node kinds are always in `vitte_ast_node_kind_t`.
- Nodes are immutable after construction except `next` and list fields.
- Module roots own import lists, export lists, declaration lists, and an `export_all` flag.
- Top-level `proc` and `const` declarations still carry an inline `exported` flag; declarations are private by default unless an export clause or `export *` marks them visible.
- Export visibility and explicit export alias resolution are centralized through `vitte_ast_decl_name`, `vitte_ast_module_find_decl`, `vitte_ast_export_decl_target`, `vitte_ast_module_decl_is_exported`, `vitte_ast_module_find_exported_decl`, and `vitte_ast_module_visit_exports`.
- Source spans are optional; valid spans keep stable source name, offsets, lines, and columns.
- Lists maintain `first`, `last`, and `count`.
- Errors use `bootstrap/src/api/error.h`.
- The AST layer does not depend on `runtime/*`.

Node kinds:
- module, import decl, export decl, proc decl, const decl
- block, give, let, if
- integer literal, string literal, identifier
- binary expr, call expr, type name
- error node

Builders in `ast.h` construct each node shape and attach list members through explicit helpers such as `vitte_ast_module_add_decl`, `vitte_ast_block_add_stmt`, and `vitte_ast_call_add_arg`.

Validation checks initialization, root presence, node kind ranges, coherent list `first`/`last`/`count`, required procedure/body fields, statement requirements, expression operands, call callees, call arguments, and depth limits.

Traversal is pre-order over all children owned by the node model. `vitte_ast_visit` stops immediately when the callback returns false and returns the number of nodes accepted by the callback before the stop.

Debug helpers are intentionally minimal:
- `vitte_ast_node_kind_name` converts a kind to a stable string.
- `vitte_ast_node_label` returns the natural label for named/labeled nodes when one exists.
- `vitte_ast_dump` writes a bounded tree dump to `FILE *` without depending on the printer module.
