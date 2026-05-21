# Vittec PHASE 1 - Professional Compiler CLI

## Quick Start

### Build simple
```bash
vittec build main.vit -o build/main
```

### Build avec traçabilité
```bash
vittec build main.vit -o build/main --trace-pipeline
```

### Check (parse + type check)
```bash
vittec check main.vit
```

### Run (build + execute)
```bash
vittec run main.vit
```

## Commandes Complètes

### check
Vérifie syntaxe + types sans générer binaire.
```bash
vittec check file.vit [--trace-pipeline]
```

**Étapes exécutées:**
1. Source manager
2. Lexer
3. Parser
4. AST validation
5. Module resolver
6. HIR lowering
7. Sema
8. Typeck
9. Borrowck

**Exit codes:**
- 0 = OK
- 1 = Erreurs utilisateur
- 2 = Erreur compilateur

### build
Build complet avec linking.
```bash
vittec build file.vit -o output [options]
```

**Étapes exécutées:** Toutes les 15 étapes

**Options:**
- `-o FILE` : Fichier sortie (défaut: a.out)
- `-O0/-O1/-O2/-O3` : Optimisation
- `--target TRIPLE` : Cible (défaut: x86_64-linux-gnu)
- `--no-debug` : Désactiver debug info
- `--trace-pipeline` : Tracer chaque étape

**Exemple:**
```bash
vittec build main.vit -o main -O2 --target aarch64-linux-gnu --trace-pipeline
```

### run
Build + exécution.
```bash
vittec run file.vit [options]
```

Les options sont les mêmes que `build`.

### dump-tokens
Affiche tokens du lexer.
```bash
vittec dump-tokens file.vit
```

### dump-ast
Affiche AST parsé.
```bash
vittec dump-ast file.vit
```

### dump-hir
Affiche HIR intermédiaire.
```bash
vittec dump-hir file.vit
```

### dump-mir
Affiche MIR intermédiaire.
```bash
vittec dump-mir file.vit
```

## Options Globales

### --trace-pipeline
Affiche chaque étape du pipeline avec temps.

```bash
vittec build main.vit --trace-pipeline
```

**Sortie:**
```
[OK] source_manager (1ms)
[OK] lexer (5ms)
[OK] parser (12ms)
[OK] ast_validation (2ms)
[OK] module_resolver (3ms)
[OK] hir_lowering (8ms)
[OK] sema (15ms)
[OK] typeck (18ms)
[OK] borrowck (10ms)
[OK] mir_lowering (7ms)
[OK] mir_verify (4ms)
[OK] optimization (22ms)
[OK] backend (35ms)
[OK] object_generation (8ms)
[OK] linker (12ms)

Compilation successful
```

### --verbose
Logging détaillé (niveau DEBUG).
```bash
vittec build main.vit --verbose
```

### --target TRIPLE
Cible compilation. Défaut: x86_64-linux-gnu

Valeurs supportées:
- x86_64-linux-gnu
- aarch64-linux-gnu
- x86_64-unknown-linux-musl
- x86_64-windows-msvc (futur)
- aarch64-darwin-gnu (futur)

```bash
vittec build main.vit --target aarch64-linux-gnu
```

### -O0, -O1, -O2, -O3
Niveaux optimisation.

- **-O0** (défaut) : Pas d'optimisation, debug info maximal
- **-O1** : Optimisations légères
- **-O2** : Optimisations standard
- **-O3** : Optimisations agressives

```bash
vittec build main.vit -O3
```

### --no-debug
Désactiver debug symbols dans binaire.
```bash
vittec build main.vit --no-debug -O3
```

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Succès |
| 1 | Erreurs utilisateur (syntax, type, etc) |
| 2 | Internal compiler error |
| 3 | Backend failure |
| 4 | Linker failure |
| 5 | I/O error |
| 6 | Invalid arguments |
| 7 | Target not supported |
| 8 | Runtime failure |

## Examples

### Simple Hello World
```bash
vittec build hello.vit -o hello
./hello
```

### Check Only
```bash
vittec check myfile.vit
```

### Optimized Build
```bash
vittec build myapp.vit -o myapp -O2 --no-debug
```

### Cross-compile ARM64
```bash
vittec build app.vit -o app.aarch64 --target aarch64-linux-gnu -O2
```

### Debug with Trace
```bash
vittec build bug.vit --trace-pipeline --verbose
```

## Diagnostics

Les erreurs sont affichées en texte lisible:

```
error: E0001: undefined symbol 'print'
error: E0002: type mismatch: expected int, got string
error: E0502: borrow conflict: cannot move while borrowed
```

Utiliser `--verbose` pour plus de détails sur chaque diagnostic.

## Performance

Temps compilation typiques (tests/pipeline/hello_world.vit):

```
source_manager   1ms
lexer            5ms
parser           12ms
typeck           18ms
borrowck         10ms
backend          35ms
linker           12ms
---
TOTAL           ~93ms
```

## Debugging

### Trace complète avec timing
```bash
vittec build file.vit --trace-pipeline --verbose 2>&1 | tee build.log
```

### Dump intermédiaires
```bash
vittec dump-ast file.vit > ast.txt
vittec dump-mir file.vit > mir.txt
```

### Check avant build
```bash
vittec check file.vit && vittec build file.vit -o binary
```

## Project Status

- ✅ Pipeline infra complète
- ✅ Source manager
- ✅ Diagnostic system
- ✅ Logging
- 🚀 Lexer (à implémenter)
- 🚀 Parser (à implémenter)
- 🚀 Type system (à implémenter)
- 🚀 Backend (à implémenter)

## Support

Pour issues/questions: https://github.com/vitte-lang/vitte/issues
