# Vitte

<div align="center">
<h3>A modern systems programming language focused on deterministic compilation, safety and self-hosting.</h3>
<p>
<img src="https://img.shields.io/badge/compiler-46%25%20complete-2563EB">
<img src="https://img.shields.io/badge/status-experimental-F97316">
<img src="https://img.shields.io/badge/license-MIT-16A34A">
<img src="https://img.shields.io/badge/bootstrap-C17-0F766E">
</p>
</div>

## Features

![status](https://img.shields.io/badge/status-experimental-F97316)
![version](https://img.shields.io/badge/version-2.1.1-2563EB)
![license](https://img.shields.io/badge/license-MIT-16A34A)
![language](https://img.shields.io/badge/language-Vitte-9333EA)
![compiler](https://img.shields.io/badge/compiler-self--hosting%20goal-DC2626)
![bootstrap](https://img.shields.io/badge/bootstrap-C17-0F766E)
![platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows-2563EB)
![frontend](https://img.shields.io/badge/frontend-Lexer%20%7C%20Parser-2563EB)
![pipeline](https://img.shields.io/badge/pipeline-AST→HIR→MIR→IR-1D4ED8)
![borrow](https://img.shields.io/badge/borrow-checker-DC2626)
![typeck](https://img.shields.io/badge/type-Type%20Checker-0F766E)
![diagnostics](https://img.shields.io/badge/diagnostics-rich-7C3AED)
![incremental](https://img.shields.io/badge/incremental-planned-F59E0B)
![llvm](https://img.shields.io/badge/backend-LLVM-4338CA)
![native](https://img.shields.io/badge/backend-Native-059669)
![object](https://img.shields.io/badge/output-Object%20Files-2563EB)
![linker](https://img.shields.io/badge/linker-integrated-0F766E)
![memory](https://img.shields.io/badge/memory-safe-16A34A)
![ownership](https://img.shields.io/badge/model-Ownership-DC2626)
![performance](https://img.shields.io/badge/performance-native-E11D48)
![generics](https://img.shields.io/badge/generics-supported-2563EB)
![async](https://img.shields.io/badge/async-supported-0F766E)
![ffi](https://img.shields.io/badge/FFI-C%20ABI-4F46E5)
![x86_64](https://img.shields.io/badge/x86__64-supported-2563EB)
![aarch64](https://img.shields.io/badge/AArch64-supported-2563EB)
![riscv64](https://img.shields.io/badge/RISC--V-supported-2563EB)
![i386](https://img.shields.io/badge/i386-supported-2563EB)
![IA-32](https://img.shields.io/badge/Architecture-IA--32-2563EB)
![Intel Pentium](https://img.shields.io/badge/CPU-Intel%20Pentium-2563EB)

Vitte is a modern systems programming language and compiler designed around explicit compilation stages, deterministic builds, memory safety, and long-term maintainability.

## Compiler progress

## Compiler Progress

Overall progress: **46%**

```text

█████████░░░░░░░░░░░ 46%

```

## Component | Status 

### Roadmap

```text

Lexer              ████░░░░░░  40%

Parser             █████░░░░░  45%

AST                █████░░░░░  50%

HIR                ██████░░░░  55%

Semantic           ████░░░░░░  35%

Type Checker       ████░░░░░░  35%

Borrow Checker     ███████░░░  70%

MIR                █████░░░░░  50%

IR                 █████░░░░░  50%

Backend            █████░░░░░  45%

LLVM               ███░░░░░░░  30%

Self Hosting       █████░░░░░  50%

```

## Repository

This repository contains the Vitte compiler, bootstrap toolchain, language grammar, tests, and documentation.

Key directories:

- `src/vitte/compiler` — compiler frontend, middle-end, backend, driver  
- `src/vitte/grammar` — language grammar source  
- `toolchain/` — bootstrap stages and workflows  
- `tests/` — regression and validation tests  
- `tools/` — scripts for checks and synchronization  
- `docs/` — documentation and reports  

Grammar source of truth: `src/vitte/grammar/vitte.ebnf`

## Pipeline

The compiler pipeline stages:

1. Lexer  
2. Parser  
3. AST validation  
4. Semantic analysis  
5. Type checking  
6. Borrow checking  
7. MIR lowering and validation  
8. IR lowering and validation  
9. Backend code generation  
10. Linking  

Failures are explicit and machine-readable to aid tooling.

## Quick start

Run a basic check:

```bash
vitte check main.vit
```

Dump diagnostics in JSON:

```bash
vitte check main.vit --diagnostics-json
```

Build a test binary:

```bash
vitte build src/vitte/compiler/tests/pipeline_tests.vit -o /tmp/vitte-pipeline-tests
```

Run main test gates:

```bash
./tools/compiler_test_suite_check_gate.sh
./tools/compiler_test_suite_bridge_gate.sh
```

Browse documentation:

- Language spec: `docs/spec/language.md`  
- Compiler docs: `docs/compiler/architecture.md`  
- Bootstrap docs: `docs/bootstrap/overview.md`  
- Generated site: `docs/index.html`  

## Example

```vitte
space hello/app

proc main() -> int {
  give 0;
}
```

## Project statistics

- Language: Vitte  
- Bootstrap: C17  
- Primary target: native executables  
- Intermediate representations: AST, HIR, MIR, IR  
- Supported architectures: x86_64, AArch64, RISC-V64, i386  
- Diagnostics: rich structured diagnostics  
- Goal: complete self-hosting compiler  

## Documentation

Key documentation:

- `docs/index.html`  
- `docs/start-here.html`  
- `docs/compiler/architecture.md`  
- `docs/compiler/pipeline.md`  
- `docs/compiler/backend.md`  
- `docs/bootstrap/overview.md`  
- `docs/spec/language.md`  

## Contributing

Contributions should be focused, explicit, and tested. Avoid hand-editing generated artifacts unless necessary.

## Summary

Vitte is an experimental systems programming language and compiler project emphasizing clear compiler engineering, deterministic builds, and maintainable systems programming.
