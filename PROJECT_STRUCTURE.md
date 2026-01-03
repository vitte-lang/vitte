# Vitte - Nouvelle Structure de Projet

## 🎯 Vue d'ensemble

```
vitte/
├── 📦 CORE (Core language infrastructure)
│   ├── bootstrap/          → Compilateur bootstrap en C
│   ├── runtime/            → Système runtime bytecode
│   └── compiler/           → Compilateur Vitte → Bytecode
│
├── 📚 REFERENCE (Documentation & Spécifications)
│   ├── docs/               → Documentation utilisateur
│   ├── spec/               → Spécifications formelles
│   └── grammar/            → Grammaire EBNF/PEST
│
├── 🧪 DEVELOPMENT (Développement & Tests)
│   ├── tests/              → Suite de tests
│   ├── examples/           → Exemples de code
│   └── benchmarks/         → Benchmarks de performance
│
├── 🛠️ TOOLING (Outils & Build)
│   ├── scripts/            → Scripts d'automatisation
│   ├── tools/              → Utilitaires et helpers
│   ├── build.sh            → Script de build principal
│   ├── CMakeLists.txt      → Configuration CMake
│   └── Makefile            → Makefile traditionnel
│
├── 📖 DOCUMENTATION (Guides)
│   ├── README.md           → Guide de démarrage
│   ├── ARCHITECTURE.md     → Architecture système
│   ├── DESIGN.md           → Décisions de design
│   ├── IMPLEMENTATION_GUIDE.md → Guide implémentation
│   └── docs/*.md           → Guides détaillés
│
├── 🎨 STANDARDS & CONFIG
│   ├── .clang-format       → Style de code
│   ├── .editorconfig       → Config éditeur
│   ├── .gitignore          → Git exclusions
│   └── doxygen.conf        → Doxygen config
│
└── 📄 METADATA (Informations projet)
    ├── VERSION             → Version actuelle
    ├── LICENSE             → Licence
    ├── CHANGELOG.md        → Historique
    └── MAINTAINERS.md      → Mainteneurs
```

## 📁 Structure Détaillée

### CORE - Infrastructure Centrale

#### bootstrap/
Compilateur de bootstrap en C (1989 LOC)
```
bootstrap/
├── README.md               → Guide bootstrap
├── CMakeLists.txt          → Build config
├── build.sh                → Build automation
├── src/                    → Code source
│   ├── lexer.c             → Tokenization
│   ├── parser.c            → Parsing
│   ├── codegen.c           → Code generation
│   └── ast.c               → AST handling
├── include/                → Headers publiques
│   ├── lexer.h
│   ├── parser.h
│   ├── codegen.h
│   └── ast.h
└── tests/                  → Tests bootstrap
```

#### runtime/
Système runtime bytecode (2451 LOC)
```
runtime/
├── README.md               → Guide runtime
├── ARCHITECTURE.md         → Architecture détaillée
├── MANIFEST.md             → Inventaire complet
├── CMakeLists.txt          → Build config
├── build.sh                → Build automation
├── config/
│   └── runtime.conf        → Configuration runtime
├── include/                → Public API
│   ├── vitte_types.h
│   └── vitte_vm.h
├── src/                    → Implémentation core
│   ├── vitte_types.c
│   ├── vitte_vm.c
│   ├── builtin.c/h
│   ├── memory.c/h
│   └── main.c
├── vm/                     → VM utilities
│   ├── debug.c/h
│   └── disassembler.c/h
├── gc/                     → Garbage collection
│   └── garbage_collector.c/h
├── abi/                    → ABI & calling conventions
│   └── calling_convention.c/h
└── tests/                  → Tests runtime
```

#### compiler/
Compilateur Vitte → Bytecode
```
compiler/
├── README.md               → Guide compilateur
├── CMakeLists.txt
├── src/
│   ├── lexer.muf/
│   ├── parser.muf/
│   ├── analyzer.muf/
│   ├── codegen.muf/
│   └── optimizer.muf/
├── include/
└── tests/
```

### REFERENCE - Documentation & Specs

#### docs/
Documentation utilisateur
```
docs/
├── README.md               → Index documentation
├── getting-started.md      → Guide démarrage
├── language-spec/          → Spécification langage
│   ├── types.md
│   ├── operators.md
│   ├── functions.md
│   ├── modules.md
│   └── statements.md
├── tutorials/              → Tutoriels
│   ├── hello-world.md
│   ├── functions.md
│   ├── modules.md
│   └── advanced.md
├── reference/              → Référence API
├── troubleshooting.md      → Dépannage
└── faq.md                  → Questions fréquentes
```

#### spec/
Spécifications formelles
```
spec/
├── README.md
├── vitte/                  → Langage Vitte
│   ├── lexical.md
│   ├── syntax.md
│   ├── semantics.md
│   ├── types.md
│   └── stdlib.md
├── runtime/                → Runtime spec
├── targets/                → Target platforms
└── muf/                    → Muffin spec
```

#### grammar/
Définitions grammaticales
```
grammar/
├── vitte.ebnf              → EBNF grammar
├── vitte.pest              → PEST grammar
├── muffin.ebnf             → Muffin grammar
└── README.md               → Guide grammaire
```

### DEVELOPMENT - Tests & Exemples

#### tests/
Suite de tests complète
```
tests/
├── README.md               → Guide tests
├── unit/                   → Tests unitaires
│   ├── types/
│   ├── vm/
│   ├── gc/
│   └── builtins/
├── integration/            → Tests intégration
│   ├── bootstrap/
│   ├── compiler/
│   └── runtime/
├── examples/               → Code exemple
│   ├── hello.vitte
│   ├── fibonacci.vitte
│   ├── sort.vitte
│   └── stdlib.vitte
└── benchmarks/             → Benchmarks performance
```

#### examples/
Exemples de code
```
examples/
├── hello/                  → Hello World
├── fibonacci/              → Fibonacci
├── file_io/                → File I/O
├── error_handling/         → Error handling
├── modules/                → Module system
├── string_processing/      → String ops
└── ffi_c/                  → FFI examples
```

### TOOLING - Build & Scripts

#### scripts/
Scripts d'automatisation
```
scripts/
├── env_local.sh            → Local environment setup
├── bootstrap_stage0.sh     → Stage 0 bootstrap
├── self_host_stage1.sh     → Self-hosted build
├── clean.sh                → Clean build
├── dev-setup.sh            → Dev environment
└── structure_project.sh    → Project structure
```

#### tools/
Utilitaires
```
tools/
├── vitte-fmt/              → Formatteur de code
├── vitte-lint/             → Linter
├── vitte-doc/              → Générateur de doc
├── debugger/               → Débuggeur
└── profiler/               → Profiler
```

### Root Level Scripts

```
build.sh                   → Build principal
CMakeLists.txt             → CMake root
Makefile                   → Makefile root
run_benchmarks.sh          → Lance benchmarks
```

## 🔗 Mappages de Dépendances

```
bootstrap/
  ↓
  → produit vittec (compilateur)
  
vittec (compilateur)
  ↓
  → compile les sources .muf en bytecode
  
compiler/ (.muf sources)
  ↓
  → produit compilateur compilé
  
runtime/
  ↓
  → VM qui exécute le bytecode
  
tests/ + examples/
  ↓
  → validation complète du système
```

## 🚀 Commandes Standards

```bash
# Build complet
./build.sh --release

# Build debug avec tests
./build.sh --debug

# Bootstrap initial
./scripts/bootstrap_stage0.sh
./scripts/self_host_stage1.sh

# Tests
./build.sh --test

# Benchmarks
./run_benchmarks.sh

# Nettoyage
./scripts/clean.sh
```

## 📊 Statistiques

| Composant | Fichiers | LOC | Status |
|-----------|----------|-----|--------|
| Bootstrap | 15 | 1,989 | ✅ Complete |
| Runtime | 22 | 2,451 | ✅ Complete |
| Compiler | TBD | TBD | 🏗️ In Progress |
| **TOTAL** | **37+** | **4,440+** | ✅ Core Ready |

## 🎯 Évolution Future

1. **Phases Immédiates**
   - Finaliser compiler/ en Muffin
   - Intégrer bootstrap ↔ compiler
   - Tests d'exécution complète

2. **Court Terme**
   - stdlib implémentation
   - Optimisations compiler
   - Documentation complète

3. **Moyen Terme**
   - JIT compilation
   - Incremental GC
   - Module system mature

4. **Long Terme**
   - Self-hosting complet
   - Optimisations avancées
   - Écosystème d'outils
