# Where To Patch What

Use this page when you know the symptom but not the right repository area.

## Compiler Symptoms

| Symptom | Start here |
| --- | --- |
| tokenization issue | `src/compiler/frontend/lexer.cpp` |
| parse or grammar issue | `src/compiler/frontend/parser.cpp` |
| diagnostics wording or location | `src/compiler/frontend/diagnostics.cpp` |
| import resolution or module loading | `src/compiler/frontend/module_loader.cpp`, `src/compiler/frontend/resolve.cpp` |
| semantic validation before lowering | `src/compiler/frontend/validate.cpp` |
| AST to HIR lowering | `src/compiler/frontend/lower_hir.cpp` |
| HIR validation or MIR lowering | `src/compiler/ir/validate.cpp`, `src/compiler/ir/lower_mir.cpp` |
| generated C++ | `src/compiler/backends/cpp_backend.cpp` |
| command behavior or pass ordering | `src/compiler/driver/pipeline.cpp`, `src/compiler/driver/passes.cpp` |

## Repository Symptoms

| Symptom | Start here |
| --- | --- |
| beginner onboarding confusion | `README.md`, `docs/GETTING_STARTED.md`, `docs/FIRST_PROJECT.md` |
| unclear public promise | `docs/PUBLIC_SURFACE.md`, `docs/COMPATIBILITY_POLICY.md` |
| release readiness confusion | `docs/RELEASE_POLICY.md`, `Makefile`, `tools/release_doctor.py` |
| package maturity confusion | `docs/PACKAGE_MATURITY.md` |
| CI discoverability problem | `docs/CI_WORKFLOWS.md`, `.github/workflows/` |
| unclear test scope | `CONTRIBUTING.md`, `docs/TEST_STRATEGY.md` |

## Rule

Patch the narrowest layer that can clearly own the behavior.
If you find yourself fixing a frontend problem in the backend, you are probably patching too late.
