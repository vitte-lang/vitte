# Architecture Overview

## Composants principaux

### Compilateur
- **Lexer** - Tokenisation
- **Parser** - Analyse syntaxique
- **Symbol Table** - Table des symboles
- **Optimizer** - Optimisation du code
- **Codegen** - Génération du bytecode
- **Linker** - Édition de liens

### Runtime
- **VM** - Machine virtuelle
- **Stack** - Pile d'exécution
- **GC** - Garbage collection
- **Memory** - Gestion mémoire
- **Executor** - Exécution des instructions

### Stdlib
- I/O, Math, String, Array, HashMap
- Time, JSON, Regex, Threading

### Debug
- Debugger, Profiler, Tracer, Breakpoints

## Flux d'exécution

```
Source Code
    ↓
Lexer (Tokens)
    ↓
Parser (AST)
    ↓
Symbol Table
    ↓
Optimizer
    ↓
Codegen (Bytecode)
    ↓
Linker
    ↓
VM Loader
    ↓
Execution
```
