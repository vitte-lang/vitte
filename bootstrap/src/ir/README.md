# bootstrap/src/ir

IR is the lower-level bootstrap intermediate representation used after HIR and
before optimization/backend-specific lowering.

## Contract

- Allocation uses `vitte_arena_t`.
- No direct `malloc` or `free` is used by this layer.
- Errors use `bootstrap/src/api/error.h`.
- No dependency on `runtime/*`.
- Values, blocks, and functions have stable non-zero IDs.
- Instructions are stored in block-local linked lists.
- Blocks must end in a terminator.
- Validation is deterministic and checks list/count coherence.
- Backend-facing IR must not contain `unknown` or `error` types.
- Numeric conversions accepted by sema are represented by explicit `cast`
  instructions before backend emission.

## Model

The module owns a linked list of global constant declarations and a linked list
of functions. Each function owns linked blocks. Each block owns linked
instructions and has a `terminated` flag.

Supported IR types:

- `void`
- `bool`
- `i32`
- `i64`
- `usize`
- `string*`
- `unknown`
- `error`

Supported instructions:

- `const_int`
- `const_string`
- `local`
- `store`
- `load`
- `cast`
- `binary`
- `call`
- `return`
- `branch`
- `cond_branch`
- `unreachable`

Instructions use a fixed small operand array. Calls currently support at most
three arguments because operand slot zero is reserved for the callee. A call
may have no result when its value is intentionally discarded, but its instruction
type still records the callee return type.

## Builder

`vitte_ir_builder_t` creates modules, functions, blocks, values, and
instructions. It tracks a current function and block. Emitting after a terminator
fails and sets the IR last error.

Typical flow:

```c
vitte_ir_builder_t builder;
vitte_ir_module_t *module;
vitte_ir_function_t *function;
vitte_ir_block_t *entry;
vitte_ir_type_t *i32;
vitte_ir_value_t *zero;

vitte_ir_builder_init(&builder, &ir);
module = vitte_ir_make_module(&builder, "main");
i32 = vitte_ir_make_type(&ir, VITTE_IR_TYPE_I32);
function = vitte_ir_make_function(&builder, "main", i32, NULL);
vitte_ir_module_add_function(module, function);
entry = vitte_ir_make_block(&builder, "entry", NULL);
vitte_ir_function_add_block(function, entry);
vitte_ir_builder_position_at_end(&builder, function, entry);
zero = vitte_ir_emit_const_int(&builder, 0, i32, NULL);
vitte_ir_emit_return(&builder, zero, NULL);
```

## Lowering

`vitte_ir_lower_hir` lowers the bootstrap HIR:

- module to module
- const declaration to global constant entry
- function declaration to function plus entry block
- block statement sequence
- return
- let as local plus optional store
- numeric `let`/`give` coercions as explicit cast instructions
- expression statement as evaluated expression with discarded result
- if as conditional branch with then/else/merge blocks
- integer and string literals
- variable as local load, global const reference, builtin const, or function reference
- binary expression
- call expression with bounded args

Global constant initializers are resolved recursively. Cycles are rejected.
Unsupported declarations or nodes return an error status instead of creating
partial silent IR.

## Validation

`vitte_ir_validate` checks:

- initialized IR and module
- globals have names, types, and resolved initializers
- functions have ids, names, return types, entry blocks
- function, global, and parameter names are unique within their owner
- blocks have ids, names, coherent instruction counts, and terminators
- instruction opcodes and operand counts
- branch targets stay inside the current function
- constants, stores, loads, casts, binary operations, calls, and returns satisfy
  their structural and type contracts
- global function/block/instruction counters

## Debug

`vitte_ir_dump` writes a compact textual module dump to `FILE *` and does not
depend on the printer module.
