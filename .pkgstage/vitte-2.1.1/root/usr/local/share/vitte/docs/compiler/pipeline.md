# Pipeline

## Overview

This page captures the observed compiler pipeline order and the specific
foundation stages that already influence frontend and lowering behavior. It is
meant to be read alongside the architecture page, but at a narrower operational
level.

| Layer | Checked responsibilities |
| --- | --- |
| Frontend | source normalization, lexer, parser, AST validation |
| Mid pipeline | symbols, visibility, type DB, HIR validate, sema, typeck |
| Control | control-flow checks, borrow checks |
| Lowering | HIR -> MIR, MIR verify, const-eval |
| Backend | backend lowering, object generation, linking |

Observed order:
1. Package/stdlib registry checks
2. Module resolution + cycle detection + duplicate module names
3. Symbol table checks + shadowing/unknowns
4. Visibility enforcement (exports/private)
5. Type database build
6. HIR validation
7. Semantic checks
8. Type checks
9. Control-flow checks
10. Borrow checks
11. HIR -> MIR
12. MIR verification
13. Const eval
14. Backend lowering + object + link

## Responsibilities

- Preserve the real execution order used by the current compiler.
- Mark which new foundation work already participates in the flow.
- Provide a compact reference for debugging stage-specific failures.

## Invariants

- Stage ordering must stay explicit and reviewable.
- Macro, async, coroutine, and concurrency foundations must be documented where
  they actually attach to the flow.
- Later roadmap ambitions must not be described as already active pipeline
  behavior.

## Data Flow

1. Input sources are normalized and tokenized.
2. Parsing and AST validation create the first structured representation.
3. Semantic layers progressively refine meaning and reject invalid contracts.
4. MIR lowering and verification prepare backend-safe intermediate state.
5. Backend lowering and link produce executable artifacts.

## Bootstrap

Bootstrap uses the same pipeline ideas under a narrower accepted subset, so
pipeline regressions often surface first in seed or stage rebuild checks.

## Driver

Driver mode selection decides which part of the pipeline becomes observable to
the user, whether through `check`, `build`, `dump-hir`, or `dump-mir`.

## Frontend Macro Stage (170)

Frontend language pipeline foundation:
1. Source normalization
2. Lexer pass (pre-expansion token baseline)
3. Macro expansion + hygiene pass
4. Lexer pass (post-expansion token stream)
5. Parser
6. AST validation

Current compiler surfaces:
- macro expansion trace string in frontend output
- macro diagnostics mapped to source spans (`line`, `column`, `width`)
- recursion limit protection to keep expansion safe

## Exemple de flux de compilation
Pour un fichier source simple, le pipeline suit ces étapes:
```vit
space app

proc main() -> void {
  let x = 1
  if x == 1 {
    give "ok"
  }
}
```
1. Résolution des modules et vérification des imports.
2. Vérification des symboles et shadowing.
3. Construction du type database.
4. Validation HIR et les vérifications sémantiques.
5. Contrôle de flux, borrow checks, puis génération MIR.
6. Vérification MIR, évaluation des constantes, et enfin abaissement backend.

## Async/Coroutine Stages (171-172)

- HIR lowering records async/await usage markers.
- HIR emits baseline misuse diagnostics for `await` outside async procedures.
- HIR->MIR lowering introduces coroutine skeleton control-flow with suspend/resume blocks for await-like statements.
- This is a foundation step; full generator frame layout and drop-order verification are tracked as follow-up work.

## Concurrency Memory Model Link (173)

Formal foundation document:
- [concurrency_memory_model.md](concurrency_memory_model.md)

## Examples

The following commands are a practical way to observe the pipeline from the
outside:

```sh
./bin/vitte check src/app.vit
./bin/vitte dump-hir src/app.vit
./bin/vitte dump-mir src/app.vit
```

These are useful when a contributor needs to determine whether a regression was
introduced during parsing, semantic checks, MIR lowering, or backend emission.
