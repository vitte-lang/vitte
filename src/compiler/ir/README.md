# compiler/src/ir

The compiler module in `ir.vitte` owns the native IR lifecycle, storage,
construction, validation, sealing, fingerprinting, and HIR to IR (the current
middle-level representation) lowering. The C17 driver passes the sealed
`IRContext` directly to `backend/c17/ir_emitter.vitte` for programs represented
by that lowering.
Nongeneric forms whose fields use scalar or other supported form/pick types now
lower through HIR and IR, including named initialization, nested field reads,
and mutable field writes. Generic or otherwise unsupported form layouts and
test declarations still use the AST emitter. Fixed-array and slice loops,
integer range loops, tuple/form/pick pattern bindings, value-producing `if`
expressions, fixed-array length queries, and statement matches use the native
IR path. Short-circuit boolean operators, optional coalescing and `try`,
assertions, and deferred statements are represented in the generated IR.
String and raw pointer iteration, async `await`, inline assembly, and `offsetof`
still require IR or target support that this layer does not yet provide.

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
- fixed-array loops with one-time iterable evaluation and break/continue targets
- slice loops with one-time iterable evaluation and break/continue targets
- nested value-producing `if` expressions whose merge block continues into
  enclosing expressions and statements
- short-circuit `and` / `or` expressions with branch-local right operands
- optional values, `??` coalescing, and `try` propagation through optional returns
- `defer` bodies on normal scope exit, return, break, and continue
- assertion failure branches ending in the backend's aborting unreachable operation
- tuple construction and nested tuple/form/pick pattern tests and bindings
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
partial silent IR. The native driver may choose its AST fallback for constructs
outside this IR model, including async tasks, inline assembly, type reflection,
and target-layout queries (`sizeof`, `alignof`, and `offsetof`). These queries
must use a target-aware lowering path instead of guessed host sizes.

## Validation

`vitte_ir_validate` checks:

- context buffers, capacities, configured limits, string ids, content hashes,
  and byte accounting are coherent
- type descriptors have kind-appropriate fields and valid element, parameter,
  return, and aggregate field types
- values have valid types and references, and parameter, instruction, function,
  global, and constant values agree with their owners
- globals have valid types and supported constant initializers
- instructions have valid result ownership, operand storage, opcode-specific
  operand counts and types, and valid aggregate, call, cast, and memory operands
- every instruction belongs to exactly one block, every produced result points
  back to its defining instruction, and recorded use counts match operands and
  phi incoming values
- instruction values and parameters are used only within their function, with
  instruction definitions preceding same-block uses and dominating reachable
  cross-block uses
- phi nodes appear before non-phi instructions and have one correctly typed
  incoming value for each predecessor
- blocks have coherent terminators and CFG edges; branch targets remain within
  their function, while predecessor and successor lists are unique and reciprocal
- functions have valid signatures and parameters, an entry block first in the
  block list, and blocks that belong to exactly one function

## Debug

`vitte_ir_dump_text` writes a compact textual module dump from validated IR
using the native Vitte formatter; it does not depend on the printer module or
an output stream. The CLI owns writing the returned text to stdout or `-o`.
