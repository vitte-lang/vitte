# Structure du Projet Vitte

## Vue d'ensemble

Ce document décrit l'organisation complète du projet **lang-vitte**, un compilateur et runtime pour le langage de programmation **Vitte**.

## Arborescence

```
lang-vitte/
├── .github/                          # Configuration GitHub
│   ├── workflows/                    # Pipelines CI/CD (GitHub Actions)
│   └── ISSUE_TEMPLATE/              # Templates pour issues/PRs
│
├── .vscode/                          # Configuration VS Code
│   ├── tasks.json                    # Tâches de build/run/debug
│   └── settings.json                 # Préférences editeur
│
├── bin/                              # Binaires et scripts wrapper
│   ├── vasm                          # Assembleur Vitte
│   ├── vbc-run                       # Runtime bytecode
│   └── vobjdump                      # Disassembleur
│
├── src/                              # Sources principales
│   ├── compiler/                     # Compilateur Vitte
│   │   ├── lexer.vitte               # Analyseur lexical
│   │   ├── parser.vitte              # Analyseur syntaxique
│   │   ├── ast.vitte                 # Arbre de syntaxe abstraite
│   │   ├── types.vitte               # Système de types
│   │   ├── ir.vitte                  # Représentation intermédiaire
│   │   ├── codegen_c.vitte          # Générateur de code C
│   │   ├── codegen_obj.vitte        # Générateur objet
│   │   ├── vbc_emit.vitte           # Émetteur bytecode
│   │   ├── driver.vitte              # Driver principal
│   │   └── buildsys.vitte            # Système de build
│   │
│   ├── runtime/                      # Runtime et ABI
│   │   ├── abi.vitte                 # Application Binary Interface
│   │   ├── vm.vitte                  # Machine virtuelle
│   │   ├── alloc.vitte               # Allocateur mémoire
│   │   ├── panic.vitte               # Gestion erreurs/panics
│   │   ├── str.vitte                 # Utilitaires string
│   │   ├── vec.vitte                 # Vecteurs dynamiques
│   │   ├── buf.vitte                 # Buffers
│   │   ├── fs.vitte                  # Système de fichiers
│   │   ├── sys.vitte                 # Appels système
│   │   └── cli.vitte                 # Interface ligne de commande
│   │
│   ├── stdlib/                       # Bibliothèque standard
│   │   ├── core/                     # Types de base et prelude
│   │   │   ├── prelude.vitte         # Re-exports automatiques
│   │   │   ├── fmt.vitte             # Formatage
│   │   │   ├── math.vitte            # Fonctions mathématiques
│   │   │   ├── mem.vitte             # Gestion mémoire
│   │   │   ├── os.vitte              # Abstractions OS
│   │   │   ├── slice.vitte           # Slices
│   │   │   └── string.vitte          # Strings
│   │   │
│   │   ├── collections/              # Structures de données
│   │   │   ├── vec.vitte             # Vecteurs
│   │   │   ├── map.vitte             # Maps/HashMaps
│   │   │   └── set.vitte             # Sets
│   │   │
│   │   ├── containers/               # Conteneurs spécialisés
│   │   │   ├── array.vitte           # Arrays fixes
│   │   │   ├── deque.vitte           # Double-ended queues
│   │   │   └── list.vitte            # Listes chaînées
│   │   │
│   │   ├── io/                       # Entrée/Sortie
│   │   │   ├── fs.vitte              # Fichiers
│   │   │   └── write.vitte           # Writers
│   │   │
│   │   ├── fs/                       # Système de fichiers avancé
│   │   │   ├── fs.vitte              # Operations FS
│   │   │   └── path.vitte            # Paths
│   │   │
│   │   ├── chrono/                   # Temps et dates
│   │   │   └── time.vitte            # Types temps
│   │   │
│   │   ├── algorithms/               # Algorithmes
│   │   │   ├── sort.vitte            # Tri
│   │   │   ├── search.vitte          # Recherche
│   │   │   └── accumulate.vitte      # Réductions
│   │   │
│   │   ├── iter/                     # Itérateurs et ranges
│   │   │   ├── traits.vitte          # Traits itérables
│   │   │   └── range.vitte           # Ranges
│   │   │
│   │   ├── random/                   # Générateurs aléatoires
│   │   │   └── rng.vitte             # RNG
│   │   │
│   │   ├── regex/                    # Expressions régulières
│   │   │   └── regex.vitte           # Regex engine
│   │   │
│   │   ├── thread/                   # Multithreading
│   │   │   ├── thread.vitte          # Threads
│   │   │   └── mutex.vitte           # Synchronisation
│   │   │
│   │   └── utility/                  # Utilitaires
│   │       ├── option.vitte          # Option<T>
│   │       ├── result.vitte          # Result<T,E>
│   │       ├── expected.vitte        # Expected<T,E>
│   │       ├── variant.vitte         # Variantes/Enums
│   │       ├── span.vitte            # Spans
│   │       └── tuple.vitte           # Tuples
│   │
│   └── tools/                        # Outils CLI
│       ├── vasm.vitte                # Assembleur
│       ├── vbc-run.vitte             # Exécuteur bytecode
│       ├── vobjdump.vitte            # Disassembleur
│       ├── vfmt.vitte                # Formatteur code
│       ├── vittec.vitte              # Compilateur
│       └── vpm.vitte                 # Package manager
│
├── tests/                            # Suite de tests
│   ├── unit/                         # Tests unitaires
│   │   ├── compiler/                 # Tests compilateur
│   │   ├── runtime/                  # Tests runtime
│   │   └── stdlib/                   # Tests stdlib
│   ├── integration/                  # Tests d'intégration
│   ├── fixtures/                     # Données/fixtures de test
│   └── samples/                      # Programmes d'exemple
│
├── examples/                         # Exemples d'utilisation
│   ├── basic/                        # Exemples basiques
│   │   ├── hello.vitte
│   │   └── fibonacci.vitte
│   ├── advanced/                     # Exemples avancés
│   │   ├── generic_sorting.vitte
│   │   └── concurrency.vitte
│   └── stdlib/                       # Exemples stdlib
│       ├── collections.vitte
│       └── async.vitte
│
├── docs/                             # Documentation
│   ├── architecture/                 # Docs architecture
│   │   ├── compiler.md               # Architecture compilateur
│   │   ├── runtime.md                # Architecture runtime
│   │   └── vm.md                     # Architecture VM
│   │
│   ├── api/                          # API documentation
│   │   ├── stdlib.md                 # API stdlib
│   │   ├── builtin.md                # Built-ins
│   │   └── abi.md                    # ABI documentation
│   │
│   ├── guides/                       # Guides utilisateur
│   │   ├── getting-started.md        # Getting started
│   │   ├── building.md               # Build instructions
│   │   └── development.md            # Development setup
│   │
│   ├── DESIGN.md                     # Décisions de design
│   ├── IR.md                         # Format IR
│   ├── VBC.md                        # Format bytecode
│   ├── SEMA.md                       # Analyse sémantique
│   └── vitte.ebnf                    # Grammaire EBNF
│
├── language/                         # Nouveau langage Vitte (specs + exemples)
│   ├── README.md                     # Vue d'ensemble de l'arborescence dédiée
│   ├── spec/                         # Spécifications textuelles
│   │   ├── lexique.md                # Identifiants, littéraux, mots-clés
│   │   ├── modules.md                # Manifestes Muffin et modules
│   │   ├── commands.md               # Commandes et déclarations avancées
│   │   ├── system.md                 # Blocs kernel/système
│   │   ├── concurrency.md            # Primitives concurrentes
│   │   ├── instrumentation.md        # Probes, meta, pragma, trace
│   │   └── extensions.md             # table/plot/ui
│   │
│   └── examples/                     # Extraits couvrant les nouveaux blocs
│       ├── modules/                  # Modules & manifestes Muffin
│       ├── system/                   # Kernels, boot, interrupt, map
│       ├── concurrency/              # Tasks, spawn, channel, race
│       ├── instrumentation/          # Probe/trace/pragma/emit
│       └── extensions/               # table/plot/ui dashboards
│
├── scripts/                          # Scripts utilitaires
│   ├── build/                        # Scripts de build
│   │   ├── build.sh                  # Build complet
│   │   ├── bootstrap.sh              # Bootstrap compilateur
│   │   └── clean.sh                  # Nettoyage
│   │
│   ├── test/                         # Scripts de test
│   │   ├── run-tests.sh              # Exécuter tests
│   │   ├── coverage.sh               # Couverture de code
│   │   └── smoke.sh                  # Tests rapides
│   │
│   └── ci/                           # Scripts CI/CD
│       ├── lint.sh                   # Linting
│       ├── format-check.sh           # Format check
│       └── release.sh                # Release
│
├── config/                           # Fichiers de configuration
│   ├── vitte.conf                    # Config par défaut
│   ├── build.conf                    # Config build
│   └── llvm.conf                     # Config LLVM (si applicable)
│
├── bootstrap/                        # Chaîne de build minimale autonome
│   ├── config/                       # Profils cibles et racines workspace
│   ├── manifest/                     # Lecture/résolution des manifests .muf
│   ├── pipeline/                     # Scheduler, rapports et orchestration
│   ├── toolchain/                    # Frontend/Backend/Linker Vitte minimal
│   ├── runtime_min/                  # Runtime minimal utilisé pendant le boot
│   ├── parser_min/                   # Lexer/Parser embarqué
│   ├── compiler_min/                 # IR + codegen/emit embarqués
│   ├── image/                        # Construction de l'image finale
│   ├── loader/                       # Nouveau loader maximal (hw, mémoire, modules)
│   ├── tools/                        # Utilitaires (info/check/dump)
│   └── vmb1/                         # Loader bas niveau + manifest
│
├── bin/                              # Binaires compilés (résultats)
│   ├── vasm
│   ├── vbc-run
│   └── vobjdump
│
├── build/                            # Artifacts build intermédiaires
│   ├── obj/                          # Fichiers objets
│   ├── lib/                          # Bibliothèques
│   └── deps/                         # Dépendances
│
├── out/                              # Outputs compilés
│   ├── *.vbc                         # Bytecode compilé
│   ├── *.o                           # Objets
│   └── executables/                  # Exécutables finaux
│
├── .github/
│   └── workflows/
│       ├── ci.yml                    # CI pipeline
│       ├── release.yml               # Release pipeline
│       └── docs.yml                  # Docs build
│
├── .gitignore                        # Git ignore patterns
├── .editorconfig                     # Configuration editeur
├── Makefile                          # Automatisation build
├── CMakeLists.txt                    # (Optionnel) Build CMake
├── LICENSE                           # License (MIT recommandé)
├── CONTRIBUTING.md                   # Guide de contribution
├── CHANGELOG.md                      # Historique changements
├── CODE_OF_CONDUCT.md                # Code de conduite
├── ROADMAP.md                        # Roadmap du projet
└── README.md                         # Readme principal
```

## Descriptions détaillées

### `/src/compiler/`
Contient le compilateur Vitte multi-passes :
- **Lexer** : Tokenization du code source
- **Parser** : Construction de l'AST
- **Type checking** : Vérification de types
- **IR generation** : Génération IR bas niveau
- **Code generation** : Backends (C, LLVM, objet natif, bytecode)

### `/src/runtime/`
Runtime et support bas niveau :
- **VM** : Machine virtuelle bytecode
- **Memory** : Gestion allocatrice/GC
- **ABI** : Convention d'appel, layout structures
- **System** : FFI vers syscalls/libs système

### `/src/stdlib/`
Bibliothèque standard Vitte répartie par catégorie :
- **core** : Types fondamentaux et prelude
- **collections** : Structures de données
- **io** : I/O et fichiers
- **algorithms** : Algorithmes génériques
- **etc.**

### `/tests/`
Suite complète de tests :
- **unit/** : Tests isolés par composant
- **integration/** : Tests de bout en bout
- **fixtures/** : Données de test et programmes attendus

### `/examples/`
Exemples d'utilisation progressifs :
- **basic/** : Hello world, boucles, fonctions
- **advanced/** : Generics, traits, async
- **stdlib/** : Utilisation stdlib

### `/docs/`
Documentation extensive :
- **architecture/** : Designs de haut niveau
- **api/** : Références API
- **guides/** : Tutoriels et how-tos

### `/scripts/`
Scripts automatisés pour :
- Build (compilation bootstrap, linking)
- Tests (exécution, coverage)
- CI/CD (lint, format, release)

### `/config/`
Fichiers de configuration centralisés pour :
- Compilation (flags, options)
- Runtime (heap size, GC, etc.)
- Outils externes (LLVM config, etc.)

## Bonnes pratiques

1. **Source code** : Toujours dans `/src/`, jamais à la racine
2. **Tests** : Colocalisés avec sources quand possible (`_test.vitte`)
3. **Docs** : Markdown dans `/docs/`, jamais dans le code
4. **Scripts** : Shell scripts organisés par fonction dans `/scripts/`
5. **Config** : Centralisée dans `/config/`, versionned
6. **Build outputs** : Jamais committé (`/build/`, `/out/` dans `.gitignore`)

## Conventions de nommage

- Fichiers source : `lowercase_with_underscores.vitte`
- Modules : `core`, `std`, `utils` (pas de underscores)
- Exécutables : `kebab-case` (vasm, vbc-run, vittec)
- Scripts : `kebab-case.sh`

## Dépendances inter-dossiers

```
src/tools/ → src/compiler/ → src/runtime/ → src/stdlib/
                ↓              ↓              ↓
            tests/compiler  tests/runtime  tests/stdlib
                ↓              ↓              ↓
         examples/ ←────────────────────────────
```

## Voir aussi

- [`CONTRIBUTING.md`](CONTRIBUTING.md) - Comment contribuer
- [`DEVELOPMENT.md`](docs/guides/development.md) - Setup dev
- [`Makefile`](Makefile) - Commandes de build courantes
