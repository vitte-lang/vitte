

muffin "vitte.project.muf"
    # =========================================================================
    # Manifest Muffin – projet principal vitte-core
    #
    # Rôle :
    #   - décrire le workspace vitte-core (lang + compiler + outils),
    #   - fournir les métadonnées (nom, version, auteur, licence),
    #   - exposer des profils de build (debug/release/test),
    #   - décrire les cibles (binaires CLI, REPL, tests),
    #   - documenter l’agencement des modules principaux.
    #
    # Remarque :
    #   - ce fichier reste purement déclaratif : aucune logique de build
    #     impérative, aucun chemin spécifique à une plateforme.
    # =========================================================================

    # -------------------------------------------------------------------------
    # Métadonnées du package vitte-core
    # -------------------------------------------------------------------------
    name: "vitte-core"
    description: "Core implementation of the Vitte language, compiler and standard library."
    version: "0.1.0-dev"
    edition: "2025"
    authors: ["Vincent Rousseau"]
    license: "MIT"
    homepage: "https://github.com/vitte-lang/vitte-core"
    repository: "https://github.com/vitte-lang/vitte-core.git"

    # Tag(s) de profil par défaut – le build system peut sélectionner un profil
    # via la clé 'profiles' ci-dessous.
    default-profile: "debug"

    # -------------------------------------------------------------------------
    # Configuration du workspace
    # -------------------------------------------------------------------------
    workspace: {
        # Racine logique du workspace (relative à ce fichier).
        root: ".",

        # Dossiers membres (lang, compiler, etc.).
        members: [
            "lang",
            "lang/std",
            "compiler",
            "compiler/src",
            "compiler/src/l"
        ]
    }

    # -------------------------------------------------------------------------
    # Configuration du langage Vitte pour ce projet
    # -------------------------------------------------------------------------
    language: {
        edition: "2025",
        dialect: "core",
        # Features activables/désactivables par le build system.
        features: [
            "hir",
            "ir",
            "llvm",
            "repl",
            "project",
            "progress"
        ]
    }

    # -------------------------------------------------------------------------
    # Profils de build (mapping symbolique -> options logiques)
    # -------------------------------------------------------------------------
    profiles: {
        "debug": {
            optimization: "debug",
            debug-info: true,
            incremental: true,
            warnings-as-errors: false,
            lto: false
        },
        "release": {
            optimization: "release",
            debug-info: false,
            incremental: false,
            warnings-as-errors: true,
            lto: true
        },
        "test": {
            optimization: "debug",
            debug-info: true,
            incremental: true,
            warnings-as-errors: false,
            lto: false
        }
    }

    # -------------------------------------------------------------------------
    # Cibles (binaires, bibliothèques, outils)
    # -------------------------------------------------------------------------
    targets: {
        "vitte": {
            kind: "binary",
            # module principal du CLI compilateur
            main: "vitte.compiler.main",
            # racine logique du projet pour cette cible
            root: "compiler/src/l",
            output-dir: "build",
            output-name: "vitte",
            profile: "debug"
        },
        "vitte-repl": {
            kind: "binary",
            main: "vitte.compiler.repl_main",
            root: "compiler/src/l",
            output-dir: "build",
            output-name: "vitte-repl",
            profile: "debug"
        },
        "vitte-tests": {
            kind: "test",
            main: "vitte.tests.main",
            root: "tests",
            output-dir: "build/tests",
            output-name: "vitte-tests",
            profile: "test"
        }
    }

    # -------------------------------------------------------------------------
    # Description logique de la std Vitte (niveau module)
    # -------------------------------------------------------------------------
    std: {
        # Modules de base (couche langage / runtime).
        modules-core: [
            "std.core",
            "std.collections",
            "std.io",
            "std.os",
            "std.math"
        ],

        # Modules utilitaires ou expérimentaux.
        modules-extra: [
            "std.fmt",
            "std.fs",
            "std.time"
        ]
    }

    # -------------------------------------------------------------------------
    # Description logique du compilateur Vitte (modules principaux)
    # -------------------------------------------------------------------------
    compiler: {
        modules-front: [
            "vitte.compiler.lexer",
            "vitte.compiler.parser",
            "vitte.compiler.ast",
            "vitte.compiler.diagnostics",
            "vitte.compiler.language",
            "vitte.compiler.path"
        ],
        modules-middle: [
            "vitte.compiler.hir",
            "vitte.compiler.ir",
            "vitte.compiler.project",
            "vitte.compiler.progress",
            "vitte.compiler.pipeline"
        ],
        modules-back: [
            "vitte.compiler.linker",
            "vitte.compiler.llvm",
            "vitte.compiler.codegen"
        ],
        modules-tools: [
            "vitte.compiler.repl",
            "vitte.compiler.lsp",
            "vitte.compiler.cli"
        ]
    }

    # -------------------------------------------------------------------------
    # Dépendances déclaratives (autres paquets / std)
    # -------------------------------------------------------------------------
    deps: [
        "std/core",
        "std/collections",
        "std/io",
        "std/os",
        "std/math"
    ]

    # -------------------------------------------------------------------------
    # Section expérimentale : hooks / outils externes
    #
    # Un système de build ou un IDE peut utiliser ces métadonnées pour proposer
    # des commandes prédéfinies (formatage, lint, benchmarks, etc.).
    # -------------------------------------------------------------------------
    tools: {
        "fmt": {
            kind: "formatter",
            command: "vitte fmt",
            description: "Format Vitte source files."
        },
        "check": {
            kind: "analyzer",
            command: "vitte check",
            description: "Run static checks without emitting binaries."
        },
        "bench": {
            kind: "benchmark",
            command: "vitte bench",
            description: "Run Vitte micro-benchmarks."
        }
    }
.end