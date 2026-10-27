# Vitte native C17 backend

This directory contains the compiler-side backend implementation in Vitte.
The bootstrap C implementation is kept under `src/bootstrap/backend/c17` and
is not linked by these modules.

## Pipeline

There are two separate native emission paths:

- `ir_emitter.vitte` accepts a sealed IR context and returns a complete C17
  source string.
- `module.vitte`, `program.vitte`, and `translation_unit.vitte` collect module
  declarations and fragments, then emit the aggregate through `backend.vitte`.
  This assembly path does not call `ir_emitter.vitte`.

`backend.vitte` owns the native C17 output state and callback API.
Its `vitte_c17_file_writer` and `vitte_c17_writer_from_file` helpers adapt a
borrowed `FILE *` to that callback API; they do not close the stream.
`writer.vitte` provides memory, callback, `FILE *`, and path sinks. Path output
uses `C17PathSink` with `vitte_c17_writer_init_path` and an explicit flush.
Reset clears the writer's own output buffer and logical counters. This also
clears memory-writer output; data already delivered to a callback or FILE
cannot be rolled back.

`ast_emitter.vitte` maps optional types to nullable C pointers (reusing pointer
representations for optional pointers, references, and strings). Form layouts
are emitted after their by-value form dependencies; pointer, reference,
optional, and dynamic-array fields do not require a complete nested layout.
By-value form cycles are rejected with a codegen error.

## IR coverage

The IR emitter maps scalar integer, boolean, floating-point, pointer, array,
slice, struct and opaque types. It emits arithmetic and comparison operations,
casts, loads, stores, address calculations, calls, string/slice operations,
conditional and direct branches, returns and unreachable blocks. String
constants use escaped byte literals; concat and substring operations include
runtime helpers only when needed. `phi` values are copied on incoming edges
using temporary snapshots, preserving parallel-copy semantics.
`switch` remains unsupported because the current IR instruction has no case
table.

The native Vitte IR declaration and C17 emitter now cover the bootstrap IR's
`Slice` type, `ConstantString` values, and six string/slice opcodes. `switch`
remains unsupported because the current IR instruction has no case table.

## Bootstrap boundary

The files here contain no `extern proc` bindings to `src/bootstrap/backend`.
The root compiler build still uses the bootstrap compiler to compile Vitte
sources; that is a separate bootstrap stage. The C files remain available as
reference until the Vitte compiler can compile itself.
