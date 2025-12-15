# Steel Language

Langage de programmation compilé performant.

## Structure du projet

- **src/** - Code source du compilateur et runtime
- **tests/** - Suite de tests
- **build/** - Fichiers compilés et scripts de build
- **docs/** - Documentation
- **tools/** - Outils et utilitaires

## Démarrage

```bash
make bootstrap  # Initialiser l'architecture
make build      # Compiler
make test       # Exécuter les tests
```

## Architecture

- **compiler/** - Lexer, Parser, Code generation
- **runtime/** - VM, Gestion mémoire
- **stdlib/** - Bibliothèque standard
