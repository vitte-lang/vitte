# Vitte 2025 — Toolchain "Steel" (portable native, single-stage)

# Steel Language

Un langage de programmation compilé, performant et expressif.

## Installation

```bash
make bootstrap  # Initialiser l'architecture
make build      # Compiler
make install    # Installer le binaire
```

## Utilisation

```bash
steel programme.steel
```

## Développement

```bash
make debug      # Compiler en mode debug
make test       # Exécuter les tests
make clean      # Nettoyer
```

## Structure du projet

```
vitte/
├── src/              # Code source
│   ├── compiler/     # Compilateur (lexer, parser, codegen)
│   ├── runtime/      # Runtime et VM
│   └── stdlib/       # Bibliothèque standard
├── include/          # En-têtes publiques
├── tests/            # Suite de tests
├── build/            # Fichiers compilés
│   └── scripts/      # Scripts de build
└── docs/             # Documentation
```

## License

MIT

Components
- `steel`  : build driver (reads `muffin.muf`, plans/executes builds)
- `steelc` : compiler (Vitte -> C17 backend) — scaffolded core
- `runtime`: minimal ABI (slices/handles/panic) — TODO
- `pal`    : Platform Abstraction Layer (POSIX/Win32) — TODO

Build model
- `muffin.muf` is mandatory. Without it, nothing builds.
- The output model mirrors Rust/Cargo intent (packages, profiles, lockfile),
  but MUF syntax is Vitte-native (`.end` blocks).

Status
- This repository is a **complete project skeleton**: directories, headers, docs, tests, fuzz, bench.
- A minimal `steelc` subset can be wired as the first milestone (parser_core + backend_c).
