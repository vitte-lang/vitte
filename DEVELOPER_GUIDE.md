# 🚀 Vitte Developer Quick Start

Bienvenue dans le projet Vitte ! Ce guide vous permet de démarrer rapidement.

## 📁 Structure du Repository

```
vitte/
├── foundation/          ← Fondations du compilateur
│   ├── core/           (erreurs, diagnostics, types de base)
│   └── utils/          (structures de données, memory)
│
├── lingua/             ← Langage (syntaxe et types)
│   ├── syntax/         (lexer, parser, AST)
│   └── types/          (type checking, traits)
│
├── compiler/           ← Compilation (IR et code gen)
│   ├── ir/             (MIR, optimisation)
│   └── backends/       (LLVM, GCC, Cranelift)
│
├── runtime/std/        ← Bibliothèque standard
├── tools/vittec/       ← Compilateur exécutable
├── docs/               ← Documentation
└── src/                ← Infrastructure de build
```

## 🆕 Créer un Nouveau Module

### Option 1: Utiliser le Template

```bash
# 1. Lire le guide
cat MODULE_TEMPLATE.md

# 2. Consulter l'exemple
ls -la EXAMPLE_MODULE/
cat EXAMPLE_MODULE/README.md

# 3. Dupliquer et adapter
cp -r EXAMPLE_MODULE foundation/utils/vitte_mymodule
cd foundation/utils/vitte_mymodule

# 4. Éditer les fichiers
# - Vitte.toml (nom, dépendances)
# - src/lib.vit (point d'entrée)
# - src/types/lib.vit (types)
# - src/impls/lib.vit (implémentations)
# - src/tests/lib.vit (tests)
# - README.md (documentation)
```

### Option 2: Guide Pas à Pas

Voir [MODULE_TEMPLATE.md](./MODULE_TEMPLATE.md) pour le guide complet.

## 🏗️ Organisation Logique

### Foundation (23 crates)
**Point d'entrée du développement de base**

Contient:
- Gestion des erreurs (`vitte_errors`)
- Positions dans le code (`vitte_span`)
- Structures de données (`vitte_data_structures`)
- Allocateurs (`vitte_arena`)

**Quand créer un module ici**:
- Types fondamentaux indépendants
- Utilitaires réutilisables
- Gestion des ressources

### Lingua (31 crates)
**Langage et sémantique**

Organisé en 2 sous-domaines:

#### syntax/ (13 crates)
- Tokenization (`vitte_lexer`)
- Parsing (`vitte_parse`)
- AST (`vitte_ast`)
- Macros (`vitte_expand`)

#### types/ (18 crates)
- Représentation HIR (`vitte_hir`)
- Type checking (`vitte_hir_typeck`)
- Traits (`vitte_trait_selection`)
- Borrow checking (`vitte_borrowck`)

**Quand créer un module ici**:
- Aspects du langage (syntaxe, types)
- Analyse sémantique
- Passes de compilation

### Compiler (14 crates)
**Code generation**

#### ir/ (4 crates)
- MIR (`vitte_mir_*`)
- Monomorphization
- Optimisation

#### backends/ (10 crates)
- Génération de code
- Support des plateformes

**Quand créer un module ici**:
- Transformation de code
- Génération pour cibles
- Optimisations bas niveau

## 📚 Conventions de Nommage

- **Crates**: `vitte_` prefix + snake_case
  - ✅ `vitte_my_module`
  - ❌ `vitte-my-module`, `my_module`

- **Types**: PascalCase
  - ✅ `MyType`, `ErrorKind`
  - ❌ `my_type`, `MYTYPE`

- **Fonctions**: snake_case
  - ✅ `process_item`, `new_builder`
  - ❌ `processItem`, `ProcessItem`

- **Constantes**: SCREAMING_SNAKE_CASE
  - ✅ `MAX_CAPACITY`, `DEFAULT_SIZE`
  - ❌ `max_capacity`, `MaxCapacity`

## 🧪 Tester un Module

```bash
# Tests du module
cargo test --package vitte_mymodule

# Tests avec affichage
cargo test --package vitte_mymodule -- --nocapture

# Test spécifique
cargo test --package vitte_mymodule my_test_name

# Vérification de compilation
cargo check --package vitte_mymodule
```

## 📋 Checklist pour Nouveau Module

- [ ] Dossier créé dans le domaine approprié
- [ ] Vitte.toml complété avec dépendances
- [ ] lib.vit avec déclarations de modules
- [ ] types/lib.vit avec structures principales
- [ ] impls/lib.vit avec implémentations
- [ ] tests/lib.vit avec au moins 5 tests
- [ ] README.md documenté
- [ ] Compilation sans erreur
- [ ] Tous les tests passent
- [ ] Dépendances minimisées

## 🔍 Explorer le Codebase

### Pour comprendre la structure:
```bash
# Vue d'ensemble
tree foundation/ -L 2 -I target

# Voir la dépendance entre modules
find lingua/syntax -name Vitte.toml -exec grep "path =" {} \;
```

### Pour trouver quelque chose:
```bash
# Chercher une fonction/type
grep -r "fn my_function" foundation/ lingua/ compiler/

# Chercher un module
find . -path "*/vitte_*/src/lib.vit" | grep mymodule

# Lister tous les modules d'un domaine
ls foundation/core/*/Vitte.toml
```

## 📖 Documentation

- [Architecture Complète](./STRUCTURE.md)
- [Migration & Historique](./MIGRATION_COMPLETE.md)
- [Module Template](./MODULE_TEMPLATE.md)
- [Exemple Concret](./EXAMPLE_MODULE/README.md)

## 🎯 Prochaines Étapes

1. **Lire** les guides mentionnés ci-dessus
2. **Explorer** la structure avec `tree` ou `find`
3. **Examiner** les modules existants dans votre domaine
4. **Copier** le template et l'adapter
5. **Écrire** votre module
6. **Tester** régulièrement
7. **Documenter** au fur et à mesure

## ❓ Questions Fréquentes

**Q: Comment ajouter une dépendance?**
A: Éditer `Vitte.toml` et ajouter dans `[dependencies]`

**Q: Quelle est la différence entre foundation/ et lingua/?**
A: foundation/ = infrastructure générale, lingua/ = langage spécifique

**Q: Dois-je créer tous les sous-dossiers?**
A: Non, adaptez la structure à vos besoins. types/ et impls/ sont suggestions.

**Q: Comment importer d'autres modules?**
A: `use vitte_other_module::{Type, function};`

---

**Bienvenue dans Vitte ! Happy coding! 🎉**
