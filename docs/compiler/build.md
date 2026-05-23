# Build & Artifacts

The seed compiler writes reproducible structured outputs under:
- `build/tokens`
- `build/ast`
- `build/hir`
- `build/mir`
- `build/c`
- `build/obj`
- `build/bin`
- `build/logs`
- `build/diagnostics`

Incremental build fingerprint includes source graph, stdlib files, and options.

## Foundation Status (174-177)

### 174. Parallel Borrow Analysis

Borrow checking now includes a concurrency-oriented foundation layer:
- Send/Sync-equivalent diagnostics for thread/send/sync misuse patterns
- shared mutation checks across thread-like boundaries
- thread ownership violation checks for escaped mutable references
- race-risk diagnostics to flag potential missing happens-before edges

Primary implementation:
- `src/vitte/compiler/analysis/borrowck/mod.vit`
- `src/vitte/compiler/analysis/borrowck/errors.vit`
- [parallel_borrow_analysis.md](/home/vincentr/Documents/GitHub/vitte/docs/compiler/parallel_borrow_analysis.md)

### 175. Advanced Optimization Passes

MIR transform foundation now tracks and reports advanced pass families:
- inline candidates
- loop simplification candidates
- branch folding candidates
- scalar replacement placeholders
- dead store elimination candidates
- value numbering placeholders

Primary implementation:
- `src/vitte/compiler/middle/mir/transform.vit`
- [advanced_optimization_passes.md](/home/vincentr/Documents/GitHub/vitte/docs/compiler/advanced_optimization_passes.md)

### 176. LLVM Backend Experimental

Experimental LLVM path is now selectable as `llvm-exp`:
- LLVM IR emission via existing LLVM binding surface
- debug metadata path preserved through profile options
- optimization bridge (`O0..Oz`, LTO/PGO compatibility checks)
- target lowering + linker/toolchain integration via LLVM toolchain status

Primary implementation:
- `src/vitte/compiler/backend/pipeline.vit`
- `src/vitte/compiler/driver/compile.vit`
- `src/vitte/compiler/backends/llvm_bindings/mod.vit`
- [llvm_backend_experimental.md](/home/vincentr/Documents/GitHub/vitte/docs/compiler/llvm_backend_experimental.md)

### 177. Native ASM Backend

Minimal native assembly backend is now selectable as `asm-native`:
- instruction selection (x86_64 path)
- virtual register allocation
- calling convention marker (`sysv-amd64`)
- stack-layout/prologue/epilogue verification signal
- object text packaging for linker integration

Primary implementation:
- `src/vitte/compiler/backend/codegen/native_asm.vit`
- `src/vitte/compiler/backend/codegen/mod.vit`
- `src/vitte/compiler/backend/pipeline.vit`
- [native_asm_backend.md](/home/vincentr/Documents/GitHub/vitte/docs/compiler/native_asm_backend.md)
