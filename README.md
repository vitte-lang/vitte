# 🎯 Lang-Vitte

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![GitHub Actions](https://github.com/lang-vitte/lang-vitte/workflows/CI/badge.svg)](https://github.com/lang-vitte/lang-vitte/actions)
[![Documentation](https://img.shields.io/badge/docs-latest-brightgreen.svg)](docs/)

**Vitte** est un langage de programmation compilé moderne, performant et sûr, avec un compilateur bootstrap complet et une machine virtuelle bytecode intégrée.

## ✨ Caractéristiques

- 🚀 **Performance** - Compilation optimisée avec plusieurs backends (C, LLVM, bytecode natif)
- 🛡️ **Sûr** - Système de types fort, gestion mémoire sans GC obligatoire
- 🎯 **Modern** - Fonctionnalités modernes : generics, traits, pattern matching
- 📚 **Stdlib riche** - Bibliothèque standard complète (collections, I/O, async, etc.)
- 🔧 **Toolchain complet** - Assembleur, disassembleur, formatteur, gestionnaire de paquets
- 🅱️ **Bytecode intermédiaire** - Format bytecode portable et optimisable

## 🚀 Démarrage rapide

```bash
# Installation
git clone https://github.com/lang-vitte/lang-vitte.git
cd lang-vitte

# Setup et build
make dev
make bootstrap
make build
make test

# Premier programme
echo 'fn main() { println("Hello, Vitte!"); }' > hello.vitte
./bin/vittec hello.vitte -o hello.vbc
./bin/vbc-run hello.vbc
```

## 📁 Structure du projet

Voir [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md) pour la description complète.

```
lang-vitte/
├── src/               # Sources principales
│   ├── compiler/      # Compilateur multi-passes
│   ├── runtime/       # Runtime et ABI
│   ├── stdlib/        # Bibliothèque standard
│   └── tools/         # Outils CLI
├── tests/             # Suite de tests complète
├── examples/          # Programmes d'exemple
├── docs/              # Documentation
├── scripts/           # Scripts build/test/CI
└── Makefile           # Automatisation
```

## 🛠️ Commandes courantes

```bash
make help          # Afficher l'aide complète
make build         # Compiler le projet
make test          # Exécuter les tests
make lint          # Vérifier la qualité
make format        # Formatter le code
make docs          # Générer la documentation
```

## 📖 Documentation

- [Getting Started](docs/guides/getting-started.md)
- [Architecture](docs/architecture/)
- [Standard Library](docs/api/stdlib.md)
- [Contributing Guide](CONTRIBUTING.md)

## 🤝 Contribuer

Voir [CONTRIBUTING.md](CONTRIBUTING.md) pour les guidelines de contribution.

## 📝 License

Sous license MIT. Voir [LICENSE](LICENSE).
