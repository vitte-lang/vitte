# Compiler Architecture (Seed Reality)

This document describes what the current seed compiler actually does.

## Overview

The current compiler architecture is organized around a deterministic driver, a
frontend and analysis pipeline, a checked HIR to MIR lowering path, and one
working backend. The goal of this page is not to describe an idealized future
compiler, but to document the contract that contributors can rely on in the
current repository state.

| Area | Current contract |
| --- | --- |
| Entry point | `toolchain/seed/vittec0.seed` |
| Driver | explicit command surface with stable modes |
| Frontend | registry, module graph, symbols, visibility, type DB |
| Analysis | HIR validate, sema, typeck, control-flow, borrowck |
| Lowering | HIR -> MIR, MIR verify, const-eval |
| Backend | C backend plus host object/link toolchain |
| Diagnostics | cataloged codes, text or JSON output |

- Front door: `toolchain/seed/vittec0.seed`.
- Modes: `check`, `build`, `run`, `test`, `dump-*`, `self-check`.
- Core pipeline in `check`:
  `registry -> module graph -> symbol/visibility -> type-db -> HIR validate ->`
  `sema -> typeck -> control-flow -> borrowck -> HIR->MIR -> MIR verify ->`
  `const-eval`.
- Backend: one functional backend (`C`), then object/link using host toolchain.
- Diagnostics: cataloged codes only, JSON or text.

## Responsibilities

This page owns the high-level architecture map:

- where the driver begins and ends,
- which phases are part of the enforced seed reality,
- which foundation tracks are already wired into the pipeline,
- which supporting specs should be consulted for narrower subsystems.

## Invariants

- The documented entry remains `toolchain/seed/vittec0.seed`.
- The checked pipeline order must stay deterministic.
- Driver and diagnostics contracts stay explicit rather than hidden behind
  fallback behavior.
- Narrow subsystem specs may evolve, but they must not silently contradict this
  architecture page.

## Data Flow

The real compiler flow currently looks like this:

1. CLI entry selects a driver mode.
2. Registry and module graph determine the project surface.
3. Frontend analysis builds symbols, visibility, and type database state.
4. HIR validation and semantic checks confirm structural correctness.
5. Type checking, control-flow, and borrow checking validate program contracts.
6. HIR lowers to MIR, MIR is verified, and constants are evaluated.
7. Backend lowering produces object files and then links with the host toolchain.

## Bootstrap

Bootstrap is part of the architecture contract because the compiler is audited
through reproducible stage transitions, not just through source organization.

- [Bootstrap overview](../bootstrap/overview.md)
- [Bootstrap stage0](../bootstrap/stage0.md)
- [Bootstrap stage1](../bootstrap/stage1.md)
- [Bootstrap stages](../bootstrap/stages.md)
- [Bootstrap reproducibility](../bootstrap/reproducibility.md)
- [Bootstrap self-host checks](../bootstrap/self_host.md)
- [Bootstrap troubleshooting](../bootstrap/troubleshooting.md)
- [Bootstrap seed contract](../bootstrap_seed.md)
- [Bootstrap native IR contract](../bootstrap_native_ir.md)

## Driver

The driver is intentionally visible in the architecture because it defines the
user-facing command boundary and CI audit surface.

- [Compiler driver alignment](../COMPILER_DRIVER_MIGRATION.md)
- [Build and release surface](build.md)
- [Release engineering](release_engineering.md)

## Foundation Status (170-173)

- `170 MACRO EXPANSION PIPELINE`
  - Frontend pipeline includes macro expansion with trace and diagnostics.
  - Macro diagnostics are mapped to frontend spans (`line`, `column`, `width`).
  - Recursion safety foundation is enforced via expansion limit diagnostics.
  - Detailed spec:
    - [macro_expansion_pipeline.md](macro_expansion_pipeline.md)
- `171 ASYNC FOUNDATION`
  - HIR lowering tags async/await usage at item level.
  - Baseline async misuse diagnostic is emitted for `await` outside async procedures.
  - Detailed spec:
    - [async_foundation.md](async_foundation.md)
- `172 COROUTINE LOWERING`
  - MIR lowering introduces suspension/resume control-flow blocks when await-like expressions are present.
  - This provides a first state-machine skeleton for future generator lowering.
  - Detailed spec:
    - [coroutine_lowering.md](coroutine_lowering.md)
- `173 CONCURRENCY MEMORY MODEL`
  - Foundation spec documented in:
    - [concurrency_memory_model.md](concurrency_memory_model.md)

Roadmap links (161-200):
- [FR pro roadmap](../roadmap_161_200_pro.md)
- [Quarterly execution plan](../roadmap_161_200_quarterly.md)
- [EN pro roadmap](../roadmap_161_200_pro_en.md)

## Examples

Un développeur qui souhaite vérifier un module utilise la commande `check` du compilateur seed :
```sh
./toolchain/seed/vittec0.seed check src/app.vit
```
Ce mode exécute les étapes principales du frontend et du pipeline de validation sans produire d'objet exécutable.
- `check` lance la résolution de module, la validation HIR, la vérification sémantique et le borrow check.
- L'étape backend est ignorée sauf pour les diagnostics qui nécessitent l'abaissement.

## Reference Map

Compiler subsystem references:

- [Pipeline](pipeline.md)
- [Type system](type-system.md)
- [Borrow checking](borrowck.md)
- [MIR](mir.md)
- [Diagnostics](diagnostics.md)
- [Diagnostics migration](diagnostics_migration.md)
- [Backend](backend.md)
- [Standard library contracts](stdlib.md)
- [Sanitizers](sanitizers.md)
- [Security limits](security_limits.md)
- [Stress and stability](stress_and_stability.md)
- [Advanced optimization passes](advanced_optimization_passes.md)
- [Parallel borrow analysis](parallel_borrow_analysis.md)
- [LLVM backend experimental notes](llvm_backend_experimental.md)
- [Native ASM backend](native_asm_backend.md)
- [Migration and editions](migration_and_editions.md)
- [Compiler power roadmap](COMPILER_POWER_ROADMAP.md)
- [Macro expansion pipeline](macro_expansion_pipeline.md)
- [Async foundation](async_foundation.md)
- [Coroutine lowering](coroutine_lowering.md)
- [Concurrency memory model](concurrency_memory_model.md)
