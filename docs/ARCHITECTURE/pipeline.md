# Pipeline du compilateur

1. **Lexing** → tokens typés, trivia conservés si besoin pour le formatter.
2. **Parsing** → AST; erreurs récupérables; mode incrémental pour LSP.
3. **Validation AST** → noms, arités, portée.
4. **Lowering** → IR (CFG/DFG), labels/stmts/ops.
5. **Passes IR** → simplif, CFG cleanup, DCE, const-fold, checks.
6. **Émission** → VITBC : index des sections, alignement, CRC, compression.
7. **Exécution** → VM/Runtime (ou target WASM).

Chaque étape loggable via `tracing` (features activables).
