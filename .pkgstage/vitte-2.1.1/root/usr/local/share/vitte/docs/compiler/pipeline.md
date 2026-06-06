# Pipeline

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
- [concurrency_memory_model.md](/home/vincentr/Documents/GitHub/vitte/docs/compiler/concurrency_memory_model.md)
