# Debugging The Compiler

This page is the shortest practical guide to debugging Vitte compiler behavior.

## Start With The Symptom

| Symptom | First place to look |
| --- | --- |
| tokenization or keyword issue | `src/compiler/frontend/lexer.cpp` |
| parse failure or grammar ambiguity | `src/compiler/frontend/parser.cpp` |
| wrong or unstable diagnostics | `src/compiler/frontend/diagnostics.cpp` |
| import or module resolution issue | `src/compiler/frontend/module_loader.cpp`, `src/compiler/frontend/resolve.cpp` |
| frontend semantic validation issue | `src/compiler/frontend/validate.cpp` |
| lowering issue between source and HIR | `src/compiler/frontend/lower_hir.cpp` |
| HIR invariant or validation issue | `src/compiler/ir/validate.cpp` |
| MIR/control-flow lowering issue | `src/compiler/ir/lower_mir.cpp` |
| generated C++ issue | `src/compiler/backends/cpp_backend.cpp` |
| command/pipeline issue | `src/compiler/driver/pipeline.cpp`, `src/compiler/driver/passes.cpp` |

## First Commands To Run

```sh
make build
make parse
make hir-validate
make core-language-gate
```

Choose the narrowest command that still reproduces the problem.

## Debugging Order

1. reproduce the issue on the smallest `.vit` file possible
2. identify the earliest compiler stage where behavior becomes wrong
3. confirm whether the issue is syntax, validation, lowering, backend, or orchestration
4. add or update a focused test before broad refactoring

## Useful Repository Anchors

- `docs/COMPILER_ARCHITECTURE.md`
- `docs/LANGUAGE_CORE_GUARANTEES.md`
- `tests/`
- `tools/`

## Practical Rule

Fix a bug in the earliest stage that can own it clearly.
Do not hide a frontend problem in backend logic or a driver workaround.
