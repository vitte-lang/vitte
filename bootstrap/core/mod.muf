# ============================================================================
# Muffin manifest – Vitte bootstrap "core" layer
#
# Ce fichier décrit la couche "core" du bootstrap Vitte, vue comme :
#   - la vision globale du workspace `vitte-core` pour le bootstrap,
#   - le regroupement logique des couches host/front/middle/pipeline,
#   - le cœur des projets (compiler, runtime, std, tools) nécessaires
#     à un bootstrap complet et auto-hébergé.
#
# Il est purement déclaratif :
#   - aucune règle de build impérative,
#   - aucune information spécifique à un autre langage que Vitte,
#   - uniquement des métadonnées pour la toolchain Vitte et les scripts.
# ============================================================================

[core]
name        = "vitte-bootstrap-core"
edition     = "2025"
description = "Core bootstrap layer and workspace view for the Vitte toolchain."
version     = "0.1.0"

# Profil par défaut pour cette couche (dev/release/ci…)
default_profile = "dev"

# ---------------------------------------------------------------------------
# Vue workspace : racine et manifests principaux
# ---------------------------------------------------------------------------

[core.workspace]
# Racine logique du workspace vitte-core (relative à ce fichier).
root = "../.."

# Manifest global du workspace (racine).
root_manifest = "muffin.muf"

# Manifests bootstrap principaux.
bootstrap_manifests = [
  "bootstrap/host/mod.muf",
  "bootstrap/front/mod.muf",
  "bootstrap/middle/mod.muf",
  "bootstrap/pipeline/mod.muf",
]

# Manifest std (bibliothèque standard).
std_manifest = "src/std/mod.muf"

# ---------------------------------------------------------------------------
# Rôles globaux de la couche core
# ---------------------------------------------------------------------------

[core.roles]
# Rôles principaux de cette couche :
#   - fournir une vue agrégée de tout le bootstrap,
#   - décrire les blocs principaux du compilateur, du runtime et de la std,
#   - servir de point d’ancrage pour les scripts et outils globaux.
roles = [
  "workspace-root",
  "bootstrap-aggregation-layer",
  "compiler-runtime-std-core",
]

# ---------------------------------------------------------------------------
# Composants bootstrap (host/front/middle/pipeline)
# ---------------------------------------------------------------------------

[core.component."bootstrap-host"]
kind        = "bootstrap-layer"
description = "Host/stage0 layer (scripts, vittec0, fixtures)."
manifest    = "bootstrap/host/mod.muf"

[core.component."bootstrap-front"]
kind        = "bootstrap-layer"
description = "Front-end layer (lex/parse/AST/diag/AST tools)."
manifest    = "bootstrap/front/mod.muf"

[core.component."bootstrap-middle"]
kind        = "bootstrap-layer"
description = "Middle layer (compiler front/middle/back, runtime core, tools)."
manifest    = "bootstrap/middle/mod.muf"

[core.component."bootstrap-pipeline"]
kind        = "bootstrap-layer"
description = "Global pipeline description for the Vitte compiler bootstrap."
manifest    = "bootstrap/pipeline/mod.muf"

# ---------------------------------------------------------------------------
# Projets cœur : compilateur, runtime, std, outils
# ---------------------------------------------------------------------------

[core.project."compiler-core"]
kind        = "library"
description = "Cœur du compilateur Vitte (front/middle/back, IR, CLI)."
root        = "src/vitte/compiler"
edition     = "2025"

modules = [
  "vitte.compiler.lexer",
  "vitte.compiler.parser",
  "vitte.compiler.ast",
  "vitte.compiler.span",
  "vitte.compiler.diagnostics",
  "vitte.compiler.scope",
  "vitte.compiler.symbols",
  "vitte.compiler.types",
  "vitte.compiler.hir",
  "vitte.compiler.mir",
  "vitte.compiler.typecheck",
  "vitte.compiler.constant_fold",
  "vitte.compiler.flow_analysis",
  "vitte.compiler.optimize",
  "vitte.compiler.ir",
  "vitte.compiler.codegen.bytecode",
  "vitte.compiler.codegen.text",
  "vitte.compiler.link",
  "vitte.compiler.cli.main",
]

[core.project."runtime-core"]
kind        = "library"
description = "Cœur du runtime/VM Vitte (VM, bytecode, GC, hooks std, CLI run)."
root        = "src/vitte/runtime"
edition     = "2025"

modules = [
  "vitte.runtime.vm",
  "vitte.runtime.bytecode",
  "vitte.runtime.gc",
  "vitte.runtime.std_hooks",
  "vitte.runtime.cli.run",
]

[core.project."tools-core"]
kind        = "tooling"
description = "Outils de base Vitte (formatter, LSP, symbol browser, test runner)."
root        = "src/vitte/tools"
edition     = "2025"

modules = [
  "vitte.tools.format",
  "vitte.tools.lsp",
  "vitte.tools.symbols",
  "vitte.tools.test_runner",
]

[core.project."std-core"]
kind        = "library"
description = "Bibliothèque standard Vitte (std) – vue globale."
root        = "src/std"
edition     = "2025"

modules = [
  "std.collections",
  "std.collections.std_vec",
  "std.collections.std_map",
  "std.fs",
  "std.fs.std_fs",
  "std.io",
  "std.io.std_io",
  "std.io.read_to_string",
  "std.path",
  "std.path.std_path",
  "std.string",
  "std.string.std_string",
  "std.time",
  "std.time.std_time",
]

# ---------------------------------------------------------------------------
# Binaries globaux vus depuis la couche core
# ---------------------------------------------------------------------------

[core.binary."vittec"]
kind        = "program"
description = "Compilateur Vitte auto-hébergé (vue logique)."
root        = "src/vitte/compiler"
entry       = "vitte.compiler.cli.main"
edition     = "2025"

requires_projects = [
  "compiler-core",
  "runtime-core",
  "std-core",
]

[core.binary."vitte-run"]
kind        = "program"
description = "Runner pour exécuter du bytecode Vitte."
root        = "src/vitte/runtime"
entry       = "vitte.runtime.cli.run"
edition     = "2025"

requires_projects = [
  "runtime-core",
  "std-core",
]

[core.binary."vitte-tools"]
kind        = "tooling-bundle"
description = "Bundle logique des outils Vitte (fmt, LSP, symbols, tests)."
root        = "src/vitte/tools"
entry       = "vitte.tools.cli.main"
edition     = "2025"

requires_projects = [
  "tools-core",
]

# ---------------------------------------------------------------------------
# Artefacts globaux produits depuis la vue core
# ---------------------------------------------------------------------------

[core.artifact."core-ir-snapshots"]
kind        = "collection"
format      = "vitte-ir-text"
description = "Dumps IR/HIR/MIR et IR final générés par le compilateur dans les scénarios de bootstrap."
output_path = "target/core/ir"

[core.artifact."core-bytecode-bundle"]
kind        = "bundle"
format      = "vitte-bytecode-bundle"
description = "Bundle logique de bytecodes Vitte (std, tests, samples) produit par le compilateur core."
output_path = "target/core/bytecode"

[core.artifact."core-test-report"]
kind        = "report"
format      = "vitte-test-report"
description = "Rapport agrégé de tests compilateur/runtime/std/tools au niveau core."
output_path = "target/core/tests/report.txt"

# ---------------------------------------------------------------------------
# Profils pour la couche core
# ---------------------------------------------------------------------------

[core.profile."dev"]
description = "Profil de développement global : tout le cœur (compiler, runtime, std, tools) est activé."
projects    = ["compiler-core", "runtime-core", "std-core", "tools-core"]
binaries    = ["vittec", "vitte-run", "vitte-tools"]
artifacts   = ["core-ir-snapshots", "core-bytecode-bundle", "core-test-report"]

[core.profile."fast-dev"]
description = "Profil de développement rapide : se concentre sur le compilateur + runtime."
projects    = ["compiler-core", "runtime-core", "std-core"]
binaries    = ["vittec", "vitte-run"]
artifacts   = ["core-ir-snapshots"]

[core.profile."ci"]
description = "Profil CI global : tous les projets, binaries et artefacts core sont activés."
projects    = ["compiler-core", "runtime-core", "std-core", "tools-core"]
binaries    = ["vittec", "vitte-run", "vitte-tools"]
artifacts   = ["core-ir-snapshots", "core-bytecode-bundle", "core-test-report"]

# ---------------------------------------------------------------------------
# Liens logiques avec les autres couches bootstrap
# ---------------------------------------------------------------------------

[core.link.bootstrap]
# Couches bootstrap constituant le cœur du bootstrap Vitte.
layers = [
  "bootstrap-host",
  "bootstrap-front",
  "bootstrap-middle",
  "bootstrap-pipeline",
]

# Relations générales (texte libre, purement documentaire).
relationship = "core aggregates host/front/middle/pipeline as a workspace-wide view."

[core.link.std]
# Référence explicite à la std.
manifest = "src/std/mod.muf"
modules  = ["std.collections", "std.fs", "std.io", "std.path", "std.string", "std.time"]

# ---------------------------------------------------------------------------
# Tooling hints – workspace et orchestration globale
# ---------------------------------------------------------------------------

[tool.vitte.core]
# Ordre recommandé d'initialisation du workspace (vue logique).
preferred_init_order = [
  "bootstrap-host",
  "bootstrap-front",
  "bootstrap-middle",
  "std-core",
  "compiler-core",
  "runtime-core",
  "tools-core",
]

# Binaries globaux considérés comme "entrées" principales pour l'utilisateur.
entry_binaries = [
  "vittec",
  "vitte-run",
  "vitte-tools",
]

[tool.vitte.core.test]
# Attentes logiques pour les tests au niveau core.
expect_vittec_success          = true
expect_core_test_report_present = true
