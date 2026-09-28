# Compiler Module Reference

The canonical compiler entry is `src/vitte/compiler/main.vit`. It delegates CLI
execution to `src/vitte/compiler/driver/compiler.vit` and the driver pipeline.

## Frontend

- `src/vitte/compiler/frontend/lexer/`: tokenization.
- `src/vitte/compiler/frontend/parse/`: parsing and recovery.
- `src/vitte/compiler/frontend/ast/`: syntax tree definitions and helpers.
- `src/vitte/compiler/frontend/macros/`: macro-facing frontend support.

## Analysis And Middle End

- `src/vitte/compiler/analysis/sema/`: semantic analysis.
- `src/vitte/compiler/analysis/typeck/`: type checking.
- `src/vitte/compiler/analysis/borrowck/`: borrow checking.
- `src/vitte/compiler/analysis/const_eval/`: constant evaluation.
- `src/vitte/compiler/middle/hir/`: high-level IR.
- `src/vitte/compiler/middle/mir/`: mid-level IR.
- `src/vitte/compiler/middle/lower/`: lowering passes.
- `src/vitte/compiler/middle/passes/`: middle-end pass orchestration.

## IR And Backends

- `src/vitte/compiler/ir/`: compiler IR contracts and transformations.
- `src/vitte/compiler/backend/`: target, code generation, linker, and backend
  interfaces.
- `src/vitte/compiler/backends/`: concrete runtime C, Vitte emit, WebAssembly,
  and LLVM binding surfaces.
- `src/vitte/compiler/optimizations/`: optimization passes.

## Driver And Infrastructure

- `src/vitte/compiler/driver/pipeline.vit`: pipeline orchestration.
- `src/vitte/compiler/driver/pipeline_modular.vit`: modular pipeline support.
- `src/vitte/compiler/driver/compile.vit`: compile request execution.
- `src/vitte/compiler/driver/commands.vit`: command handling.
- `src/vitte/compiler/diagnostics/`: compiler diagnostics.
- `src/vitte/compiler/infrastructure/`: sessions, incremental state,
  distribution, and diagnostic infrastructure.

## Validation

```sh
make compiler-architecture-check
make compiler-real-pipeline-audit
make compiler-test-suite-check-gate
make compiler-no-fallback-gate
```

Generated architecture reports live under `target/reports/`; they are evidence
for a specific run, not source-of-truth modules.
