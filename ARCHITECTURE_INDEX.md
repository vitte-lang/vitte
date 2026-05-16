# Vitte Compiler Architecture Index

Start here:

1. [ARCHITECTURE_COMPLETE.md](ARCHITECTURE_COMPLETE.md) - current technical architecture.
2. [COMPILER_ROADMAP.md](COMPILER_ROADMAP.md) - feature roadmap and proof gates.
3. [COMPILER_GRAMMAR_ALIGNMENT_ANALYSIS.md](COMPILER_GRAMMAR_ALIGNMENT_ANALYSIS.md) - parser/grammar status.
4. [COMPILER_GRAMMAR_ALIGNMENT_INTEGRATION.md](COMPILER_GRAMMAR_ALIGNMENT_INTEGRATION.md) - grammar integration workflow.
5. [COMPILER_GRAMMAR_ALIGNMENT_ROADMAP.md](COMPILER_GRAMMAR_ALIGNMENT_ROADMAP.md) - next parser/AST milestones.
6. [DELIVERY_SUMMARY.md](DELIVERY_SUMMARY.md) - concise delivery summary.

## Real Module Map

| Area | Paths | Gate |
|---|---|---|
| Frontend lexer/parser | `src/vitte/compiler/frontend/lexer/`, `src/vitte/compiler/frontend/parse/` | `make frontend-syntax-test` |
| Frontend AST surface | `src/vitte/compiler/frontend/ast/` | `make grammar-alignment-test` |
| Grammar alignment | `src/vitte/compiler/frontend/grammar_alignment_checker.vit`, `tools/grammar_alignment_checker.py` | `make grammar-alignment-test` |
| IR surface | `src/vitte/compiler/ir/ast.vit`, `src/vitte/compiler/ir/mir_extended.vit` | `make compiler-gate` |
| HIR/MIR middle | `src/vitte/compiler/middle/hir/`, `src/vitte/compiler/middle/mir/`, `src/vitte/compiler/middle/lower/` | `make compiler-gate` |
| Analysis | `src/vitte/compiler/analysis/`, `src/vitte/compiler/middle/borrow/`, `src/vitte/compiler/middle/infer/` | `make analysis-gate` |
| Vitte emit backend | `src/vitte/compiler/backends/vitte_emit/` | `make vitte-emit-gate` |
| LLVM/WASM backends | `src/vitte/compiler/codegen/llvm/`, `src/vitte/compiler/codegen/wasm/`, `src/vitte/compiler/backends/wasm/` | `make llvm-backend-gate`, `make wasm-backend-gate` |
| Driver runtime | `src/vitte/compiler/driver/` | `make driver-report-runtime-test` |
| Bootstrap | `toolchain/`, `scripts/seed/`, `toolchain/scripts/bootstrap/` | `make test` |
| Ecosystem tools | `src/vitte/tools/`, `src/vitte/compiler/performance/` | `make roadmap-ecosystem-gate` |

## Architecture Flow

```text
.vit/.vitl source
  -> frontend/lexer/scanner.vit
  -> frontend/parse/parser.vit
  -> frontend/pipeline.vit
  -> analysis/pipeline.vit
  -> middle HIR/MIR and optimization modules
  -> backend IR/codegen/link modules
  -> vitte_emit / LLVM / WASM artifacts
```

## Current High-Signal Reports

- `target/reports/vitte_emit_coverage.md`
- `target/reports/llvm_backend_coverage.md`
- `target/reports/wasm_backend_coverage.md`
- `target/reports/static_analysis_coverage.md`
- `target/reports/type_system_coverage.md`
- `target/reports/memory_model_coverage.md`
- `target/reports/concurrency_model_coverage.md`
- `target/reports/roadmap_ecosystem_coverage.md`

## Anti-Drift Rule

Architecture docs must not point contributors to obsolete detached extended-AST/HIR files or the old flat frontend parser path. Use the real paths listed above.
