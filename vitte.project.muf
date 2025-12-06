# ============================================================================
# Vitte project manifest – vitte-core
#
# Ce fichier complète le manifest racine `muffin.muf` en fournissant une vue
# "projet" plus détaillée de vitte-core, orientée toolchain Vitte :
#
#   - métadonnées du projet principal,
#   - description des sous-projets logiques (compiler, runtime, std, tools),
#   - profils (dev/release/ci) au niveau projet,
#   - liens avec le bootstrap et la std,
#   - options spécifiques pour la toolchain Vitte.
#
# Il est purement déclaratif et ne contient aucun script ni règle de build.
# ============================================================================

[project]
name        = "vitte-core"
edition     = "2025"
version     = "0.1.0"
description = "Core language, compiler, runtime, std and tooling for Vitte."

# Type logique du projet (workspace principal de la toolchain Vitte).
kind        = "workspace"

# Racine logique du projet (relative à ce fichier).
root        = "."

# Manifest workspace principal (racine).
workspace_manifest = "muffin.muf"

# Manifest bootstrap agrégé.
bootstrap_manifest = "bootstrap/mod.muf"

# Manifest std agrégé.
std_manifest = "src/std/mod.muf"

# ---------------------------------------------------------------------------
# Sous-projets logiques du repo vitte-core
# (vue "projet" plus fine que le manifest workspace)
# ---------------------------------------------------------------------------

[project.subproject."compiler"]
kind        = "library"
description = "Vitte compiler core (front/middle/back, CLI)."
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

[project.subproject."runtime"]
kind        = "library"
description = "Vitte runtime core (VM, bytecode, GC, std hooks, CLI run)."
root        = "src/vitte/runtime"
edition     = "2025"

modules = [
  "vitte.runtime.vm",
  "vitte.runtime.bytecode",
  "vitte.runtime.gc",
  "vitte.runtime.std_hooks",
  "vitte.runtime.cli.run",
]

[project.subproject."tools"]
kind        = "tooling"
description = "Vitte tools (formatter, LSP, symbol browser, test runner, CLI)."
root        = "src/vitte/tools"
edition     = "2025"

modules = [
  "vitte.tools.format",
  "vitte.tools.lsp",
  "vitte.tools.symbols",
  "vitte.tools.test_runner",
  "vitte.tools.cli.main",
]

[project.subproject."std"]
kind        = "library"
description = "Vitte standard library (std) – logical bundle."
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

[project.subproject."bootstrap"]
kind        = "bootstrap-meta"
description = "Bootstrap-related metadata (host, front, middle, back, cli, core, pipeline)."
root        = "bootstrap"
edition     = "2025"

manifests = [
  "bootstrap/mod.muf",
  "bootstrap/host/mod.muf",
  "bootstrap/front/mod.muf",
  "bootstrap/middle/mod.muf",
  "bootstrap/back/mod.muf",
  "bootstrap/cli/mod.muf",
  "bootstrap/core/mod.muf",
  "bootstrap/pipeline/mod.muf",
]

[project.subproject."tests"]
kind        = "tests"
description = "Tests, fixtures and sample projects for the Vitte toolchain."
root        = "tests"
edition     = "2025"

paths = [
  "tests/data",
  "tests/data/mini_project",
  "tests/data/samples",
  "tests/data/lex",
  "tests/data/parse",
]

# ---------------------------------------------------------------------------
# Profils projet – dev / release / ci
# ---------------------------------------------------------------------------

[project.profile."dev"]
description = "Profil de développement : tout le core (compiler/runtime/std/tools/bootstrap/tests)."
subprojects = ["compiler", "runtime", "std", "tools", "bootstrap", "tests"]
edition     = "2025"

[project.profile."release"]
description = "Profil release : focus sur compiler/runtime/std/tools en mode publié."
subprojects = ["compiler", "runtime", "std", "tools"]
edition     = "2025"

[project.profile."ci"]
description = "Profil CI : inclut l’ensemble des sous-projets, y compris bootstrap et tests."
subprojects = ["compiler", "runtime", "std", "tools", "bootstrap", "tests"]
edition     = "2025"

# ---------------------------------------------------------------------------
# Dépendances logiques (entre sous-projets)
# ---------------------------------------------------------------------------

[project.deps."compiler"]
# Le compilateur dépend de la std (contrats, modèles) et des tests.
depends_on = ["std", "tests"]

[project.deps."runtime"]
depends_on = ["std", "tests"]

[project.deps."tools"]
depends_on = ["compiler", "runtime", "std"]

[project.deps."std"]
depends_on = []

[project.deps."bootstrap"]
depends_on = ["compiler", "runtime", "std", "tools"]

[project.deps."tests"]
depends_on = ["compiler", "runtime", "std", "tools"]

# ---------------------------------------------------------------------------
# Entrées principales (binaries logiques)
# ---------------------------------------------------------------------------

[project.entry."vittec"]
kind        = "program"
description = "Main Vitte compiler binary."
subproject  = "compiler"
entry       = "vitte.compiler.cli.main"

[project.entry."vitte-run"]
kind        = "program"
description = "Main Vitte runtime/runner."
subproject  = "runtime"
entry       = "vitte.runtime.cli.run"

[project.entry."vitte-tools"]
kind        = "program"
description = "Bundle d’outils Vitte (formatter, LSP, symbols, tests…)."
subproject  = "tools"
entry       = "vitte.tools.cli.main"

[project.entry."vitte-bootstrap"]
kind        = "program"
description = "Commande pour piloter le bootstrap Vitte (stages, reports)."
subproject  = "bootstrap"
entry       = "bootstrap.cli.entry"

# ---------------------------------------------------------------------------
# Tooling – paramètres spécifiques pour la toolchain Vitte
# ---------------------------------------------------------------------------

[tool.vitte.project]
# Edition par défaut utilisée par les outils (CLI, LSP, etc.).
default_edition = "2025"

# Profil par défaut lorsque non spécifié (dev/release/ci).
default_profile = "dev"

# Std à charger par défaut pour ce projet.
std_manifest = "src/std/mod.muf"

# Bootstrap à considérer pour les opérations liées aux stages.
bootstrap_manifest = "bootstrap/mod.muf"

# Liste d’entrées "publiques" suggérées aux UIs/outils.
entry_binaries = [
  "vittec",
  "vitte-run",
  "vitte-tools",
  "vitte-bootstrap",
]

[tool.vitte.project.test]
# Attentes logiques globales au niveau projet.
expect_compiler_present = true
expect_runtime_present  = true
expect_std_present      = true
expect_tools_present    = true
expect_tests_present    = true
