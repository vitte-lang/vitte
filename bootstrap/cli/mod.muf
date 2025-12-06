# ============================================================================
# Muffin manifest – Vitte bootstrap "cli" layer
#
# Ce fichier décrit la couche "cli" du bootstrap Vitte :
#   - commandes en ligne de commande (vittec, vitte-run, vitte-tools, etc.),
#   - projets logiques contenant le code CLI (par-dessus compiler/runtime/tools),
#   - artefacts produits pour l’ergonomie CLI (help, complétions, docs),
#   - liens avec les autres couches de bootstrap et la vue core.
#
# Il est purement déclaratif :
#   - aucune règle de build impérative,
#   - aucune dépendance explicite à d’autres langages que Vitte,
#   - uniquement des métadonnées pour la toolchain Vitte et les scripts.
# ============================================================================

[cli]
name        = "vitte-bootstrap-cli"
edition     = "2025"
description = "CLI bootstrap layer for the Vitte toolchain (vittec, vitte-run, vitte-tools)."
version     = "0.1.0"

# Profil par défaut
default_profile = "dev"

# ---------------------------------------------------------------------------
# Rôles globaux de la couche CLI
# ---------------------------------------------------------------------------

[cli.roles]
# Rôles principaux de cette couche :
#   - fournir les points d’entrée CLI pour le compilateur, le runtime et les outils,
#   - encapsuler les options, le parsing des arguments et les messages d’aide,
#   - générer les artefacts utiles à l’ergonomie (help-text, complétions, docs).
roles = [
  "cli-entry-points",
  "cli-argument-layer",
  "cli-ergonomics-provider",
]

# ---------------------------------------------------------------------------
# Projets logiques CLI
# ---------------------------------------------------------------------------

[cli.project."compiler-cli"]
kind        = "tooling"
description = "Couche CLI du compilateur Vitte (vittec, sous-commandes, options)."
root        = "src/vitte/compiler/cli"
edition     = "2025"

modules = [
  "vitte.compiler.cli.main",
  "vitte.compiler.cli.args",
  "vitte.compiler.cli.subcommands",
  "vitte.compiler.cli.diagnostics",
]

[cli.project."runtime-cli"]
kind        = "tooling"
description = "Couche CLI du runtime Vitte (vitte-run, inspection de bytecode, etc.)."
root        = "src/vitte/runtime/cli"
edition     = "2025"

modules = [
  "vitte.runtime.cli.run",
  "vitte.runtime.cli.inspect",
  "vitte.runtime.cli.args",
]

[cli.project."tools-cli"]
kind        = "tooling"
description = "Couche CLI des outils Vitte (formatter, LSP, symbol browser, test runner)."
root        = "src/vitte/tools/cli"
edition     = "2025"

modules = [
  "vitte.tools.cli.main",
  "vitte.tools.cli.format",
  "vitte.tools.cli.lsp",
  "vitte.tools.cli.symbols",
  "vitte.tools.cli.test",
]

[cli.project."bootstrap-cli"]
kind        = "tooling"
description = "Commandes CLI de support pour le bootstrap (wrapper autour des scripts et stages)."
root        = "bootstrap/cli"
edition     = "2025"

modules = [
  "bootstrap.cli.entry",
  "bootstrap.cli.stages",
  "bootstrap.cli.env",
]

# ---------------------------------------------------------------------------
# Binaries CLI (vues logiques)
# ---------------------------------------------------------------------------

[cli.binary."vittec"]
kind        = "program"
description = "Commande principale du compilateur Vitte (vittec)."
root        = "src/vitte/compiler/cli"
entry       = "vitte.compiler.cli.main"
edition     = "2025"

requires_projects = [
  "compiler-cli",
]

[cli.binary."vitte-run"]
kind        = "program"
description = "Commande pour exécuter du bytecode Vitte (vitte-run)."
root        = "src/vitte/runtime/cli"
entry       = "vitte.runtime.cli.run"
edition     = "2025"

requires_projects = [
  "runtime-cli",
]

[cli.binary."vitte-tools"]
kind        = "program"
description = "Commande pour regrouper les outils Vitte (formatter, LSP, symbols, test)."
root        = "src/vitte/tools/cli"
entry       = "vitte.tools.cli.main"
edition     = "2025"

requires_projects = [
  "tools-cli",
]

[cli.binary."vitte-bootstrap"]
kind        = "program"
description = "Commande pour piloter les stages de bootstrap Vitte (wrapper autour des scripts)."
root        = "bootstrap/cli"
entry       = "bootstrap.cli.entry"
edition     = "2025"

requires_projects = [
  "bootstrap-cli",
]

# ---------------------------------------------------------------------------
# Artefacts CLI – help, complétions, docs
# ---------------------------------------------------------------------------

[cli.artifact."help-text"]
kind        = "collection"
format      = "vitte-cli-help"
description = "Textes d'aide CLI (usage, sous-commandes, options) générés à partir des binaries."
output_path = "target/bootstrap/cli/help"

[cli.artifact."shell-completions"]
kind        = "collection"
format      = "vitte-cli-completions"
description = "Scripts de complétion (zsh, bash, fish, etc.) pour les commandes Vitte."
output_path = "target/bootstrap/cli/completions"

[cli.artifact."cli-usage-report"]
kind        = "report"
format      = "vitte-cli-report"
description = "Rapport logique sur les chemins de code CLI exercés par la suite de tests."
output_path = "target/bootstrap/cli/report.txt"

# ---------------------------------------------------------------------------
# Profils pour la couche CLI
# ---------------------------------------------------------------------------

[cli.profile."dev"]
description = "Profil de développement CLI : toutes les commandes et artefacts CLI sont activés."
projects    = ["compiler-cli", "runtime-cli", "tools-cli", "bootstrap-cli"]
binaries    = ["vittec", "vitte-run", "vitte-tools", "vitte-bootstrap"]
artifacts   = ["help-text", "shell-completions", "cli-usage-report"]

[cli.profile."fast-dev"]
description = "Profil de développement rapide : se concentre sur vittec et vitte-run."
projects    = ["compiler-cli", "runtime-cli"]
binaries    = ["vittec", "vitte-run"]
artifacts   = ["help-text"]

[cli.profile."ci"]
description = "Profil CI CLI : vérifie toutes les commandes et génère les artefacts d'ergonomie."
projects    = ["compiler-cli", "runtime-cli", "tools-cli", "bootstrap-cli"]
binaries    = ["vittec", "vitte-run", "vitte-tools", "vitte-bootstrap"]
artifacts   = ["help-text", "shell-completions", "cli-usage-report"]

# ---------------------------------------------------------------------------
# Liens logiques avec les autres couches bootstrap et la vue core
# ---------------------------------------------------------------------------

[cli.link.core]
# Référence logique vers la vue core du workspace (bootstrap/core/mod.muf).
core_name = "vitte-bootstrap-core"

# Binaries core auxquels la CLI correspond (vue globale).
core_binaries = [
  "vittec",
  "vitte-run",
  "vitte-tools",
]

[cli.link.bootstrap]
# Couches bootstrap auxquelles la CLI est connectée.
layers = [
  "bootstrap-host",
  "bootstrap-front",
  "bootstrap-middle",
  "bootstrap-pipeline",
]

# ---------------------------------------------------------------------------
# Tooling hints – scripts et toolchain Vitte
# ---------------------------------------------------------------------------

[tool.vitte.cli]
# Ordre recommandé de mise à disposition des commandes dans l'environnement.
preferred_install_order = [
  "vittec",
  "vitte-run",
  "vitte-tools",
  "vitte-bootstrap",
]

# Programmes considérés comme "faces" publiques principales de la toolchain Vitte.
entry_binaries = [
  "vittec",
  "vitte-run",
  "vitte-tools",
  "vitte-bootstrap",
]

[tool.vitte.cli.test]
# Attentes logiques pour les tests CLI.
expect_vittec_help_success     = true
expect_vitte_run_help_success  = true
expect_cli_help_text_present   = true
expect_cli_completions_present = true
