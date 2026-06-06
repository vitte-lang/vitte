# Memory Safety Audit - Vitte Compiler

Date: 2026-05-23
Scope: compiler internals, backend/runtime interfaces, runtime C support, ownership and lifetime analysis.

## 1. Unsafe Inventory

### 1.1 Language-level `unsafe` surface
- `unsafe` is parsed as a block construct in the frontend parser: [parser.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/frontend/parse/parser.vit:852).
- AST carries `unsafe` flags in items/statements:
  - proc signatures default to `unsafe: false`: [item.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/frontend/ast/item.vit:299)
  - statements mark `AstStmtKind.Unsafe`: [stmt.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/frontend/ast/stmt.vit:463)
- Finding: there is parsing/modeling support, but no explicit end-to-end enforcement boundary in this audit scope that would require unsafe ops to be inside unsafe blocks.
- Risk: **Medium** (policy drift risk, not immediate memory corruption by itself).

### 1.2 Raw pointers and deref typing
- Raw pointer kinds are present in AST type system (`RawPointer`): [type_expr.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/frontend/ast/type_expr.vit:38).
- Pointer unification exists (`*const`, `*mut`) in typeck: [unify.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/analysis/typeck/unify.vit:237).
- Finding: pointer type compatibility is handled; lifetime/provenance guarantees for raw pointers are not established by type unification alone.
- Risk: **Medium**.

### 1.3 Native/C boundary usage
- Real heap allocation primitives appear in C runtime (`malloc`, `realloc`, `calloc`): [vitte_runtime.c](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/backends/runtime_c/vitte_runtime.c:80).
- Backend declares runtime stubs `vitte_alloc/vitte_free/vitte_print`: [llvm_emit.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/backends/llvm_emit.vit:109), [c_emit.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/backends/c_emit.vit:103).
- Finding: declarations exist, but corresponding implementations are not present in `runtime_c` header/source exported API shown in this audit.
- Risk: **High** (ABI/link mismatch, potential undefined behavior if symbol expectations diverge across paths).

## 2. Ownership Audit

### 2.1 Strengths
- Ownership model tracks local states (`Available`, `Moved`, borrow counters): [ownership.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/analysis/borrowck/ownership.vit:11).
- Place overlap/prefix checks exist for field/index/deref projections: [ownership.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/analysis/borrowck/ownership.vit:171).
- Borrow checker enforces move-while-borrowed and write-while-borrowed diagnostics: [mod.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/analysis/borrowck/mod.vit:714), [mod.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/analysis/borrowck/mod.vit:748).

### 2.2 Gaps / risks
- Borrow metadata scopes are line-based approximations (`scope_end_line`) and sentinel max values: [mod.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/analysis/borrowck/mod.vit:319), [mod.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/analysis/borrowck/mod.vit:322).
- This can be conservative or imprecise for complex control-flow/lifetime nesting.
- Risk: **Medium** (false positives/negatives possible at edges).

## 3. Allocator Audit

### 3.1 Runtime C allocator behavior
- `realloc`-based push for slices and `malloc` for strings: [vitte_runtime.c](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/backends/runtime_c/vitte_runtime.c:78), [vitte_runtime.c](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/backends/runtime_c/vitte_runtime.c:102).
- Panic marker set on allocation failure (`vitte_note_panic(3)`), but no global ownership/free API for all allocations in this unit.
- `cli_args` allocates with `calloc` and returns owning slice without matching runtime free primitive exposed here: [vitte_runtime.c](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/backends/runtime_c/vitte_runtime.c:150).
- Risk: **High** for leak accumulation in long-lived processes.

### 3.2 Core allocator model quality
- `stdlib/core/memory.vitl` provides a bookkeeping allocator model (good for reasoning/tests): [memory.vitl](/home/vincentr/Documents/GitHub/vitte/src/vitte/stdlib/core/memory.vitl:73).
- But arithmetic is unchecked for underflow/overflow during allocate/release accounting (`free - size`, `used - block.size`): [memory.vitl](/home/vincentr/Documents/GitHub/vitte/src/vitte/stdlib/core/memory.vitl:101), [memory.vitl](/home/vincentr/Documents/GitHub/vitte/src/vitte/stdlib/core/memory.vitl:137).
- Risk: **Medium** (model integrity bug; if reused in production-like contexts this becomes high).

## 4. Pointer Lifetime Audit

### 4.1 Runtime pointer lifetime issues
- `VitteString` uses `{const char *data, size_t len}` and string constructors allocate buffers: [vitte_runtime.h](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/backends/runtime_c/vitte_runtime.h:11), [vitte_runtime.c](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/backends/runtime_c/vitte_runtime.c:102).
- No audited destructor/ownership transfer API for `VitteString`/`VitteSliceString` heap buffers.
- Risk: **High** (lifetime leaks and ambiguous ownership).

### 4.2 Compiler IR-level lifetime validation
- MIR validation checks CFG integrity and terminator consistency: [validate.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/middle/mir/validate.vit:16).
- IR verification checks structural validity of instructions/blocks/modules: [ir.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/backend/ir/ir.vit:630), [ir.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/backend/ir/ir.vit:731).
- Finding: structural checks are strong, but they do not prove pointer provenance/liveness at machine boundary.
- Risk: **Medium**.

## 5. Backend Memory Safety Review

### 5.1 LLVM backend
- Emits intrinsic declarations including lifetime intrinsics: [llvm_emit.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/backends/llvm_emit.vit:103).
- Current emission body is placeholder-oriented (`; statement`, simplistic terminator mapping): [llvm_emit.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/backends/llvm_emit.vit:157).
- Risk: **Medium** now (limited feature implementation), **High** once codegen becomes complete without deeper safety contracts.

### 5.2 C backend
- Runtime declarations in generated C do not currently align with runtime header signatures (`vitte_print` type mismatch: declared with `vitte_string_t` in emitter vs absent in runtime header): [c_emit.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/backends/c_emit.vit:105), [vitte_runtime.h](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/backends/runtime_c/vitte_runtime.h:26).
- Risk: **High** (ABI mismatch and call-site UB risk).

### 5.3 Linker/object pipeline
- Linker/object code is largely pseudo-model text assembly, reducing direct memory corruption surface in these modules: [linker.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/backend/link/linker.vit:1), [object.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/backend/codegen/object.vit:1).
- Risk: **Low** in current form.

## 6. Runtime Memory Review

### 6.1 Panic boundary semantics
- Thread-local panic state with boundary depth is clean and bounded in logic: [vitte_runtime.c](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/backends/runtime_c/vitte_runtime.c:7).
- `vitte_builtin_trap` aborts outside boundary, otherwise marks panic state: [vitte_runtime.c](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/backends/runtime_c/vitte_runtime.c:55).
- Risk: **Low** for memory safety; mostly control-flow behavior.

### 6.2 Memory ownership API incompleteness
- No explicit exported free/destroy functions for heap-backed string/slice values created by runtime helpers in this audit scope.
- Combined with repeated concat/push patterns, this is leak-prone.
- Risk: **High**.

## 7. Priority Findings (Top)

1. **High**: Backend/runtime ABI mismatch for allocation/print symbols (`vitte_alloc/vitte_free/vitte_print`) across emitters vs runtime C exports.
2. **High**: Runtime heap objects (`VitteString`, `VitteSliceString`) lack explicit destruction/ownership-release API in audited surface.
3. **High**: `cli_args` heap allocation lifecycle is unpaired with teardown function.
4. **Medium**: Borrow/lifetime metadata is line/scope heuristic based, not full CFG region model.
5. **Medium**: Core allocator bookkeeping performs unchecked arithmetic underflow/overflow.

## 8. Recommended Remediation Plan

### Phase A (blocking, immediate)
- Define and implement canonical runtime ABI in one place:
  - Add concrete `vitte_alloc`, `vitte_free`, `vitte_print` implementations in `runtime_c` (or remove declarations from emitters if unused).
  - Ensure signatures are identical across:
    - [llvm_emit.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/backends/llvm_emit.vit:109)
    - [c_emit.vit](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/backends/c_emit.vit:103)
    - [vitte_runtime.h](/home/vincentr/Documents/GitHub/vitte/src/vitte/compiler/backends/runtime_c/vitte_runtime.h:26)
- Add destructors:
  - `vitte_string_free(VitteString)`
  - `vitte_slice_string_free(VitteSliceString, bool free_elements)`
  - `vitte_slice_i32_free(VitteSliceI32)`

### Phase B (short term)
- Add overflow guards before allocations:
  - `next_len > SIZE_MAX / sizeof(T)` checks before `realloc`.
  - `a.len + b.len` overflow checks before `malloc(total + 1)`.
- Add runtime fuzz/ASan/UBSan jobs for `runtime_c`.
- Add negative tests for ABI mismatch detection during build.

### Phase C (medium term)
- Tighten unsafe policy:
  - Require explicit unsafe context for raw pointer deref operations.
  - Emit diagnostics when unsafe operations appear outside unsafe scope.
- Evolve borrow/lifetime analysis toward CFG/region precision rather than line-end heuristics only.

## 9. Audit Conclusion

Current compiler core (HIR/MIR/IR validation + borrow machinery) has solid structural safety foundations, but memory safety risk is concentrated at the backend/runtime native boundary. The most urgent issues are ABI consistency and explicit deallocation ownership APIs in `runtime_c`.
