# LLVM Backend Validation

Generated: 2026-07-01T00:23:52.615598+00:00

## Summary

- Status: **PASS**
- Required files: 4
- Required symbols: 6
- Missing files: 0
- Missing symbols: 0

## Files

| File | Exists |
|------|--------|
| src/vitte/compiler/backends/llvm_bindings/mod.vit | PASS |
| src/vitte/compiler/backends/llvm_bindings/tests/smoke.vit | PASS |
| src/vitte/compiler/backends/vitte_emit/mod.vit | PASS |
| tools/llvm/generate_artifacts.py | PASS |

## Symbols

| Symbol | Present |
|--------|---------|
| supported_opt_levels | PASS |
| emit_llvm_ir_from_mir | PASS |
| debug_info_format | PASS |
| pgo_enabled | PASS |
| emit_debug_info | PASS |
| emit_pgo_instrumentation | PASS |
