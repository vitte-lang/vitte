# PHASE 1 - Quick Start (2 min)

## Installation

Aucune installation requise. Le code est dans:
```
src/vitte/compiler/infrastructure/session/
src/vitte/compiler/driver/
tests/
```

## Commandes Basiques

### Build
```bash
vittec build main.vit -o main
```

### Check (sans linker)
```bash
vittec check main.vit
```

### Run
```bash
vittec run main.vit
```

## Options Utiles

### Trace Pipeline (voir chaque étape)
```bash
vittec build main.vit --trace-pipeline
```

Output:
```
[OK] source_manager (1ms)
[TODO] lexer (0ms)
[TODO] parser (0ms)
...
```

### Verbose (logs détaillés)
```bash
vittec build main.vit --verbose
```

### Dump AST
```bash
vittec dump-ast main.vit
```

## Exit Codes

```
0 = Succès
1 = Erreurs utilisateur
2 = Internal compiler error
3 = Backend failure
4 = Linker failure
```

## Exemples

### Fichier simple
```bash
# hello.vit
proc main() { }

# Compiler
vittec build hello.vit -o hello --trace-pipeline
```

### Avec options
```bash
vittec build app.vit -o app -O2 --target aarch64-linux-gnu --no-debug
```

## Test Framework

Les tests se trouvent dans `tests/pipeline/`:
```
hello_world.vit       ← OK case
syntax_error.vit      ← Parser fail
type_error.vit        ← Typeck fail
borrow_error.vit      ← Borrowck fail
backend_failure.vit   ← Backend fail
```

Exécuter les tests:
```bash
vittec build tests/pipeline/hello_world.vit --trace-pipeline
```

## Documentation Complète

- `COMPILER_CLI_PHASE1_README.md` - Guide complet
- `PHASE_1_IMPLEMENTATION.md` - Détails techniques
- `PHASE_1_TODO.md` - Quoi faire après

## Architecture Rapide

```
CLI Input
    ↓
Parse arguments (cli_options.vit)
    ↓
Create CompilationSession
    ↓
Run strict_pipeline (15 étapes)
    ↓
Return exit code (0-8)
```

## Intégrer dans PHASE 2

Quand on implémente le lexer:

```vit
// Dans strict_pipeline.vit, remplacer:
set mut_logger = log_warn(mut_logger, "pipeline", "lexer phase: UNIMPLEMENTED");
set mut_session = record_phase_execution(
    mut_session,
    PipelinePhase::Lexer,
    PhaseResult::Unimplemented,
    0,
    "Lexer not yet implemented"
);

// Avec:
let (lexed_session, lex_result) = lexer_run(mut_session);
set mut_session = lexed_session;
set mut_session = record_phase_execution(
    mut_session,
    PipelinePhase::Lexer,
    lex_result,
    duration,
    "Lexer completed"
);
```

## Points Clés

1. **Traçabilité**: `--trace-pipeline` montre chaque étape
2. **Erreurs**: Stoppées automatiquement si fatales
3. **Logging**: 5 niveaux (trace/debug/info/warn/error)
4. **Multi-platform**: x86_64, aarch64, etc.
5. **Professional**: UNIX exit codes, lisible

## Troubleshooting

### Build fails avec "critical phases unimplemented"
C'est normal! Les phases réelles ne sont pas implémentées. C'est PHASE 2.

### Voir détails d'erreur
```bash
vittec build file.vit --verbose --trace-pipeline
```

### Vérifier syntax
```bash
vittec dump-ast file.vit  # Dump AST (si parser implémenté)
```

---

**Status**: PHASE 1 Complete ✅  
**Next**: Implement phases (PHASE 2)
