# Compiler Architecture (Seed Reality)

This document describes what the current seed compiler actually does.

- Front door: `toolchain/seed/vittec0.seed`.
- Modes: `check`, `build`, `run`, `test`, `dump-*`, `self-check`.
- Core pipeline in `check`: registry -> module graph -> symbol/visibility -> type-db -> HIR validate -> sema -> typeck -> control-flow -> borrowck -> HIR->MIR -> MIR verify -> const-eval.
- Backend: one functional backend (`C`), then object/link using host toolchain.
- Diagnostics: cataloged codes only, JSON or text.

## Foundation Status (170-173)

- `170 MACRO EXPANSION PIPELINE`
  - Frontend pipeline includes macro expansion with trace and diagnostics.
  - Macro diagnostics are mapped to frontend spans (`line`, `column`, `width`).
  - Recursion safety foundation is enforced via expansion limit diagnostics.
  - Detailed spec:
    - [macro_expansion_pipeline.md](/home/vincentr/Documents/GitHub/vitte/docs/compiler/macro_expansion_pipeline.md)
- `171 ASYNC FOUNDATION`
  - HIR lowering tags async/await usage at item level.
  - Baseline async misuse diagnostic is emitted for `await` outside async procedures.
  - Detailed spec:
    - [async_foundation.md](/home/vincentr/Documents/GitHub/vitte/docs/compiler/async_foundation.md)
- `172 COROUTINE LOWERING`
  - MIR lowering introduces suspension/resume control-flow blocks when await-like expressions are present.
  - This provides a first state-machine skeleton for future generator lowering.
  - Detailed spec:
    - [coroutine_lowering.md](/home/vincentr/Documents/GitHub/vitte/docs/compiler/coroutine_lowering.md)
- `173 CONCURRENCY MEMORY MODEL`
  - Foundation spec documented in:
    - [concurrency_memory_model.md](/home/vincentr/Documents/GitHub/vitte/docs/compiler/concurrency_memory_model.md)

Roadmap links (161-200):
- [FR pro roadmap](/home/vincentr/Documents/GitHub/vitte/docs/roadmap_161_200_pro.md)
- [Quarterly execution plan](/home/vincentr/Documents/GitHub/vitte/docs/roadmap_161_200_quarterly.md)
- [EN pro roadmap](/home/vincentr/Documents/GitHub/vitte/docs/roadmap_161_200_pro_en.md)

## Exemple d'utilisation
Un développeur qui souhaite vérifier un module utilise la commande `check` du compilateur seed :
```sh
./toolchain/seed/vittec0.seed check src/app.vit
```
Ce mode exécute les étapes principales du frontend et du pipeline de validation sans produire d'objet exécutable.
- `check` lance la résolution de module, la validation HIR, la vérification sémantique et le borrow check.
- L'étape backend est ignorée sauf pour les diagnostics qui nécessitent l'abaissement.
