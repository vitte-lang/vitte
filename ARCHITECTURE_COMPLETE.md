# Vitte Compiler Architecture

## Executive Summary

This document describes the compiler architecture as it exists in the repository today. Older notes referenced detached extended-AST/HIR files and a flat frontend parser path; those paths are obsolete.

Current architecture:

```text
source .vit/.vitl
  -> frontend lexer/scanner
  -> frontend parse/parser.vit
  -> frontend AST surface definitions
  -> middle HIR/MIR pipelines
  -> backend IR/codegen/link pipelines
  -> Vitte-native, LLVM, WASM, and runtime artifacts
```

Core validation gates:

- `make grammar-alignment-test`
- `make frontend-syntax-test`
- `make roadmap-ecosystem-gate`
- `make compiler-gate`
- `make test`

---

## Frontend

Location: `src/vitte/compiler/frontend/`

Responsibilities:

- Source input normalization.
- Token scanning.
- Grammar-surface parsing and recovery.
- AST surface definitions.
- Macro expansion hooks.
- Frontend diagnostics emitted into `FrontendOutput`.

Important modules:

| Module | Purpose |
|---|---|
| `input.vit` | Source input envelope |
| `lexer/token.vit` | Token kinds and token form |
| `lexer/scanner.vit` | Token stream production |
| `parse/parser.vit` | Structural parser for declarations, statements, expressions, types, patterns, attributes, and recovery |
| `parse/recovery.vit` | Diagnostic capping and recovery state |
| `ast/*.vit` | AST kind and node surface definitions |
| `grammar_alignment_checker.vit` | In-tree grammar alignment metadata |
| `pipeline.vit` | `frontend_run` orchestration |

Key parser status:

- Declarations: 32/32 tracked rules complete and tested.
- Statements: 24/24 tracked rules complete and tested.
- Expressions: 24/24 tracked rules complete and tested.
- Types: 14/14 tracked rules complete and tested.
- Patterns: 11/11 tracked rules complete and tested.

Executable proof:

```sh
make grammar-alignment-test
make frontend-syntax-test
```

Current limitation:

- The parser walks and validates the structural grammar and returns counters/diagnostics. Rich typed AST materialization is the next compiler milestone, rather than a missing parser.

---

## AST Surface

Frontend AST surface:

- `src/vitte/compiler/frontend/ast/node.vit`
- `src/vitte/compiler/frontend/ast/item.vit`
- `src/vitte/compiler/frontend/ast/expr.vit`
- `src/vitte/compiler/frontend/ast/stmt.vit`
- `src/vitte/compiler/frontend/ast/type_expr.vit`
- `src/vitte/compiler/frontend/ast/pattern.vit`
- `src/vitte/compiler/frontend/ast/validate.vit`

IR AST surface:

- `src/vitte/compiler/ir/ast.vit`

Coverage:

- Declaration kinds include space/use/export, const/static/global, type/opaque/extern, form/class/union/bits/pick/flags, trait/impl, proc/intrinsic/query, compiler/pass/backend/diagnostic/macro, comptime/static_assert/test/bench/entry.
- Statement kinds include local const, let, set, give, try, defer, asm, unsafe, emit, assert, panic, unreachable, if, while, loop, for, break, continue, select, match, when-match, with, critical, expr.
- Expression kinds include literals, paths, calls, members, indexes, unary/binary/assign/cast/is/range/ternary, struct/list/tuple/set/map/bytes/resource literals, if/match/lambda/proc/unsafe/block/builtin/raw.
- Type kinds include named, primitive, qualified, reference, pointer, optional, fixed array, slice, tuple, proc, dyn, impl trait, generic, union, lifetime, raw.
- Pattern kinds include bind, constructor, struct, tuple, list, range, or, mut, ref, wildcard, literal, raw.

---

## Middle IR

Locations:

- `src/vitte/compiler/middle/hir/`
- `src/vitte/compiler/middle/mir/`
- `src/vitte/compiler/middle/lower/`
- `src/vitte/compiler/middle/optimizations/`
- `src/vitte/compiler/ir/mir_extended.vit`
- `src/vitte/compiler/ir/hir_to_mir_lowering.vit`
- `src/vitte/compiler/ir/mir_optimizations.vit`

Responsibilities:

- HIR construction and validation.
- MIR construction and validation.
- HIR -> MIR and MIR -> backend IR lowering.
- Constant propagation, copy propagation, dead code elimination, inlining, CFG simplification.

Related gates:

- `make type-system-gate`
- `make memory-model-gate`
- `make concurrency-model-gate`
- `make mir-opt-gate`
- `make interproc-opt-gate`
- `make static-analysis-gate`

---

## Analysis

Locations:

- `src/vitte/compiler/analysis/pipeline.vit`
- `src/vitte/compiler/analysis/borrowck/`
- `src/vitte/compiler/analysis/typeck/traits.vit`
- `src/vitte/compiler/middle/borrow/`
- `src/vitte/compiler/middle/infer/`

Responsibilities:

- Analysis pipeline status.
- Borrow, ownership, move, loan, region, and lifetime models.
- Trait checking surface.
- Inference and constraints surface.

Related gates:

- `make analysis-gate`
- `make type-system-gate`
- `make memory-model-gate`

---

## Backends

Locations:

- `src/vitte/compiler/backends/vitte_emit/`
- `src/vitte/compiler/backends/llvm_emit.vit`
- `src/vitte/compiler/backends/wasm/`
- `src/vitte/compiler/backend/ir/`
- `src/vitte/compiler/backend/codegen/`
- `src/vitte/compiler/backend/link/`
- `src/vitte/compiler/backend/target/`
- `src/vitte/compiler/codegen/llvm/`
- `src/vitte/compiler/codegen/wasm/`

Backend responsibilities:

- Vitte-native textual IR and assembly-like artifacts.
- ABI metadata and module export manifests.
- LLVM and WASM emission surfaces.
- Backend IR verification.
- Target triples, layouts, features, and link artifact models.

Current backend policy:

- The project is intentionally Vitte-first and avoids generating `.c/.h` as the primary path.
- `c_emit.vit` exists as a backend surface, but the roadmap/gates focus on Vitte-native emit, LLVM, and WASM.

Related gates:

- `make vitte-emit-gate`
- `make llvm-backend-gate`
- `make wasm-backend-gate`
- `make backend-gate`
- `make ffi-abi-gate`

---

## Driver And Pipelines

Locations:

- `src/vitte/compiler/driver/compile.vit`
- `src/vitte/compiler/driver/pipeline.vit`
- `src/vitte/compiler/driver/compilation_pipeline.vit`
- `src/vitte/compiler/driver/bootstrap_pipeline.vit`
- `src/vitte/compiler/driver/cli.vit`
- `src/vitte/compiler/driver/mod.vit`

Responsibilities:

- CLI command routing.
- Check/compile report generation.
- Frontend -> analysis -> backend orchestration.
- Bootstrap pipeline metadata.
- JSON report contracts for driver runtime tests.

Related gates:

- `make driver-report-runtime-test`
- `make bootstrap-verify`
- `make bootstrap-native-snapshots`
- `make test`

---

## Bootstrap

Bootstrap implementation lives mostly under:

- `toolchain/`
- `scripts/seed/`
- `toolchain/scripts/bootstrap/`
- `src/vitte/compiler/driver/bootstrap_pipeline.vit`

Current practical bootstrap path:

- Seed compiler installs `bin/vittec0`.
- Stage 1 builds/verifies `vittec1`.
- Stage 2 builds/verifies final `vittec`.
- Snapshot and parity gates verify reproducibility and emitted artifacts.

Executable proof:

```sh
make bootstrap-verify
make bootstrap-native-snapshots
make test
```

---

## Tooling And Ecosystem

Locations:

- `src/vitte/tools/lsp/`
- `src/vitte/tools/ide/`
- `src/vitte/tools/debugger/`
- `src/vitte/tools/profiler/`
- `src/vitte/tools/coverage/`
- `src/vitte/compiler/performance/`
- `src/vitte/stdlib/network/`
- `src/vitte/stdlib/crypto/`
- `src/vitte/stdlib/json/`

Coverage:

- LSP surface.
- VS Code-style IDE manifest surface.
- Native debugger model.
- Integrated profiler model.
- Coverage report model.
- Networking, cryptography, JSON serialization.
- Parallel compilation, memory pooling, JIT and AOT planning surfaces.
- Linux, macOS, Windows, embedded, and WASM evidence.

Executable proof:

```sh
make roadmap-ecosystem-gate
```

---

## File Structure

```text
src/vitte/compiler/
  frontend/
    lexer/
    parse/
    ast/
    macros/
    pipeline.vit
    grammar_alignment_checker.vit
  middle/
    hir/
    mir/
    lower/
    borrow/
    infer/
    optimizations/
  analysis/
    borrowck/
    typeck/
    pipeline.vit
  ir/
    ast.vit
    mir_extended.vit
    hir_to_mir_lowering.vit
    mir_optimizations.vit
    pipeline.vit
  backend/
    ir/
    codegen/
    link/
    target/
  backends/
    vitte_emit/
    wasm/
    llvm_emit.vit
    c_emit.vit
    backend_infrastructure.vit
  driver/
    compile.vit
    pipeline.vit
    compilation_pipeline.vit
    bootstrap_pipeline.vit
```

---

## Definition Of Done Gates

The architecture is considered coherent when these pass:

```sh
make grammar-alignment-test
make frontend-syntax-test
make driver-report-runtime-test
make roadmap-ecosystem-gate
make compiler-gate
make test
```

These gates verify the current architecture more reliably than static claims about line counts or estimated timings.

---

## Current Next Milestones

1. Rich AST materialization from parser walks.
2. AST -> HIR lowering over the richer AST node forms.
3. More negative fixtures for attributes, `select`, byte literals, generic params, and recovery edges.
4. Startup-time optimization proof and gate.
5. Memory-safety verification proof and gate.
6. Larger test-count accounting if the project wants to claim `1000+` tests.

---

## Documentation Version

Updated May 16, 2026.
