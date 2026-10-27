# compiler/src/ir

The compiler module in `ir.vitte` now owns the native IR lifecycle, storage,
construction, validation, sealing, fingerprinting, and the supported HIR to IR
lowering. The C17 driver passes the sealed `IRContext` directly to
`backend/c17/ir_emitter.vitte` for programs represented by that lowering.
Nongeneric forms whose fields use scalar or other supported form/pick types now
lower through HIR and IR, including named initialization, nested field reads,
and mutable field writes. Generic or otherwise unsupported form layouts,
non-range `for` iterables, and test declarations still use the AST emitter.
Integer range loops with a simple name or wildcard binding, value-producing
`if` expressions, and statement matches also use the native IR path.

IR is the lower-level compiler intermediate representation used after HIR and
before optimization/backend-specific lowering.

## Contract

- Allocation uses `vitte_arena_t`.
- No direct `malloc` or `free` is used by this layer.
- Errors use `compiler/src/api/error.h`.
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

The native IR type model includes:

- `void`
- `bool`
- signed and unsigned integers from 8 to 64 bits
- `f32`, `f64`, and `char`
- pointers, fixed arrays, slices, functions, named structs, opaque types, and picks

Aggregate construction and projection use `struct_make`, `struct_field`,
`load`, and `store`. The IR also supports array and pick operations, control
flow, calls, string helpers, and numeric conversions.

Common instructions include:

- `const_int`
- `const_string`
- `alloca`
- `array_make`
- `struct_make`
- `struct_field`
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

`vitte_ir_lower_hir` lowers the compiler HIR:

- module to module
- const declaration to global constant entry
- function declaration to function plus entry block
- block statement sequence
- return
- let as local plus optional store
- numeric `let`/`return` coercions as explicit cast instructions
- expression statement as evaluated expression with discarded result
- if as conditional branch with then/else/merge blocks
- value-producing if as branch-local stores and a merge load
- exclusive and inclusive integer range loops with break/continue targets
- nested value-producing `if` expressions whose merge block continues into
  enclosing expressions and statements
- integer and string literals
- variable as local load, global const reference, builtin const, or function reference
- binary expression
- call expression with bounded args
- nongeneric form declarations and named form initializers
- form member reads and assignable field projections, including nested forms
- fixed-array construction and indexed access
- scalar and payload pick construction, tag tests, and payload extraction

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

`vitte_ir_dump_text` writes a compact textual module dump from validated IR
using the native Vitte formatter; it does not depend on the printer module or
an output stream. The CLI owns writing the returned text to stdout or `-o`.
