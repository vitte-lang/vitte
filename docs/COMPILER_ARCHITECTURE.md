# Compiler Architecture

This document is the shortest repository-level map of how the Vitte compiler is laid out.

Use it when you need to answer one of these questions:

- where should I start reading?
- where should I patch a given behavior?
- which stage owns a specific invariant?

## Pipeline Overview

The current compiler flow is:

```text
source text
  -> lexer
  -> parser
  -> AST/frontend validation
  -> HIR lowering
  -> MIR lowering
  -> backend code emission
  -> native toolchain
```

Repository anchors:

- `src/compiler/frontend`
- `src/compiler/ir`
- `src/compiler/backends`
- `src/compiler/driver`
- `src/compiler/linker`

## Directory Responsibilities

### `src/compiler/frontend`

This layer owns source-facing work:

- lexing
- parsing
- AST construction
- source spans
- diagnostics
- name/module loading and early validation
- lowering from frontend structures toward HIR

Good starting files:

- `src/compiler/frontend/lexer.cpp`
- `src/compiler/frontend/parser.cpp`
- `src/compiler/frontend/diagnostics.cpp`
- `src/compiler/frontend/module_loader.cpp`
- `src/compiler/frontend/validate.cpp`
- `src/compiler/frontend/lower_hir.cpp`

If a bug is about syntax, spans, parser recovery, import/module loading, or frontend diagnostics, start here.

### `src/compiler/ir`

This layer owns normalized compiler representations:

- HIR structures and validation
- MIR structures and lowering
- IR builder helpers

Good starting files:

- `src/compiler/ir/hir.cpp`
- `src/compiler/ir/validate.cpp`
- `src/compiler/ir/lower_mir.cpp`
- `src/compiler/ir/mir.cpp`

If a bug is about normalized control flow, intermediate invariants, or lowering between compiler stages, start here.

### `src/compiler/backends`

This layer owns target-facing emission.

Current repository docs and README references describe the main backend flow as C++ emission followed by the native toolchain.

If a bug is about generated C++, backend lowering, or emit behavior, start in:

- `src/compiler/backends/cpp_backend.cpp`

### `src/compiler/driver`

This layer owns orchestration:

- CLI options
- pass ordering
- end-to-end pipeline execution
- build/check/parse command behavior

Good starting files:

- `src/compiler/driver/options.cpp`
- `src/compiler/driver/passes.cpp`
- `src/compiler/driver/pipeline.cpp`
- `src/compiler/driver/vittec.cpp`

If a bug is about command behavior, pass ordering, or CLI surface, start here.

### `src/compiler/linker`

This layer owns linking and object/archive handling.

If a bug is about linking, archives, symbols, or final artifact assembly, start in:

- `src/compiler/linker/linker.cpp`
- `src/compiler/linker/archive.cpp`
- `src/compiler/linker/symbols.cpp`

## Practical Entry Points

Use this shortcut map when choosing where to begin:

| You want to change... | Start in... |
| --- | --- |
| tokens, keywords, lexing errors | `src/compiler/frontend/lexer.cpp` |
| grammar, parse rules, syntax recovery | `src/compiler/frontend/parser.cpp` |
| source error wording and spans | `src/compiler/frontend/diagnostics.cpp` |
| import/module resolution | `src/compiler/frontend/module_loader.cpp` and `src/compiler/frontend/resolve.cpp` |
| semantic validation before lowering | `src/compiler/frontend/validate.cpp` |
| AST to HIR lowering | `src/compiler/frontend/lower_hir.cpp` |
| HIR invariants | `src/compiler/ir/validate.cpp` |
| HIR to MIR lowering | `src/compiler/ir/lower_mir.cpp` |
| backend C++ emission | `src/compiler/backends/cpp_backend.cpp` |
| driver command flow | `src/compiler/driver/pipeline.cpp` and `src/compiler/driver/passes.cpp` |

## Stage Invariants

A useful mental model:

- frontend should preserve source fidelity and deterministic diagnostics
- HIR should make high-level structure easier to validate and transform
- MIR should normalize control flow and operations for backend work
- backend should emit target code without re-deciding frontend language semantics
- driver should orchestrate stages, not quietly redefine their contracts

If a behavior is ambiguous, prefer fixing it in the earliest stage that can own it clearly.

## How To Read The Compiler

A practical reading order for new contributors:

1. `README.md`
2. `docs/LANGUAGE_CORE.md`
3. `src/compiler/frontend/README.md`
4. `src/compiler/frontend/parser.cpp`
5. `src/compiler/frontend/validate.cpp`
6. `src/compiler/frontend/lower_hir.cpp`
7. `src/compiler/ir/lower_mir.cpp`
8. `src/compiler/backends/cpp_backend.cpp`
9. `src/compiler/driver/pipeline.cpp`

## Related Docs

- `README.md`
- `docs/GETTING_STARTED.md`
- `docs/LANGUAGE_CORE.md`
- `src/compiler/frontend/README.md`
