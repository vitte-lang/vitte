# ============================================================================
# Muffin manifest – Vitte bootstrap "front" layer
#
# Ce fichier décrit la couche "front" du bootstrap Vitte :
#   - composants du front-end du compilateur (lexer, parser, AST, spans, diag),
#   - projets logiques construits autour de ces composants,
#   - binaries/outils front-end utilisés pour la validation (ast-dump, front-check),
#   - artefacts intermédiaires produits (AST, diags, traces de parse).
#
# Il est purement déclaratif :
#   - aucune règle de build impérative,
#   - aucune dépendance explicite sur un langage autre que Vitte,
#   - uniquement des métadonnées pour la toolchain Vitte et les scripts.
# ============================================================================

[front]
name        = "vitte-bootstrap-front"
edition     = "2025"
description = "Front-end bootstrap layer for the Vitte compiler (lex/parse/AST/diag)."
version     = "0.1.0"

# Profil par défaut pour cette couche (dev/release/ci…)
default_profile = "dev"

# ---------------------------------------------------------------------------
# Rôles globaux de la couche front
# ---------------------------------------------------------------------------

[front.roles]
# Rôles principaux de la couche front :
#   - fournir un front-end Vitte cohérent (lexer, parser, AST, spans, diagnostics),
#   - produire des artefacts AST/stats/diags pour le bootstrap et les tests,
#   - servir de base commune aux couches "middle" et "host" pour les validations.
roles = [
  "front-end-core",
  "ast-and-diag-provider",
  "syntax-validation-layer",
]

# ---------------------------------------------------------------------------
# Projets logiques de la couche front
# ---------------------------------------------------------------------------

[front.project."compiler-front-core"]
kind        = "library"
description = "Composants cœur du front-end Vitte (lexer, parser, AST, spans, diagnostics)."
root        = "src/vitte/compiler/front"
edition     = "2025"

modules = [
  "vitte.compiler.lexer",
  "vitte.compiler.parser",
  "vitte.compiler.ast",
  "vitte.compiler.span",
  "vitte.compiler.diagnostics",
]

[front.project."compiler-front-scope"]
kind        = "library"
description = "Gestion des scopes, symboles et première résolution de noms côté front-end."
root        = "src/vitte/compiler/front"
edition     = "2025"

modules = [
  "vitte.compiler.scope",
  "vitte.compiler.symbols",
]

[front.project."compiler-front-tools"]
kind        = "tooling"
description = "Outils front-end (AST dumper, parse tracer, diag viewer)."
root        = "src/vitte/compiler/front/tools"
edition     = "2025"

modules = [
  "vitte.compiler.front.ast_dump",
  "vitte.compiler.front.parse_trace",
  "vitte.compiler.front.diag_viewer",
]

# ---------------------------------------------------------------------------
# Binaries/outils front-end
# ---------------------------------------------------------------------------

[front.binary."vitte-front-check"]
kind        = "program"
description = "Outil de vérification front-end (lex/parse/AST/diag) utilisé dans le bootstrap."
root        = "src/vitte/compiler/front"
entry       = "vitte.compiler.front.cli.check"
edition     = "2025"

requires_projects = [
  "compiler-front-core",
  "compiler-front-scope",
]

[front.binary."vitte-ast-dump"]
kind        = "program"
description = "Outil pour dumper les AST Vitte (debug, tests, introspection)."
root        = "src/vitte/compiler/front/tools"
entry       = "vitte.compiler.front.cli.ast_dump"
edition     = "2025"

requires_projects = [
  "compiler-front-core",
  "compiler-front-tools",
]

[front.binary."vitte-parse-trace"]
kind        = "program"
description = "Outil pour tracer le parsing et produire des rapports détaillés."
root        = "src/vitte/compiler/front/tools"
entry       = "vitte.compiler.front.cli.parse_trace"
edition     = "2025"

requires_projects = [
  "compiler-front-core",
  "compiler-front-tools",
]

# ---------------------------------------------------------------------------
# Artefacts front-end produits par cette couche
# ---------------------------------------------------------------------------

[front.artifact."ast-dumps"]
kind        = "collection"
format      = "vitte-ast-text"
description = "Dumps d'AST textuels produits par vitte-ast-dump pour tests et debug."
output_path = "target/bootstrap/front/ast"

[front.artifact."parse-traces"]
kind        = "collection"
format      = "vitte-parse-trace"
description = "Traces détaillées de parsing, utilisées pour le debug et la validation."
output_path = "target/bootstrap/front/parse_traces"

[front.artifact."front-diag-report"]
kind        = "report"
format      = "vitte-diag-report"
description = "Rapport de diagnostics front-end (erreurs/avertissements lex/parse/scope)."
output_path = "target/bootstrap/front/diag/report.txt"

# ---------------------------------------------------------------------------
# Profils pour la couche front
# ---------------------------------------------------------------------------

[front.profile."dev"]
description = "Profil de développement front : tous les projets, binaries et artefacts front sont activés."
projects    = ["compiler-front-core", "compiler-front-scope", "compiler-front-tools"]
binaries    = ["vitte-front-check", "vitte-ast-dump", "vitte-parse-trace"]
artifacts   = ["ast-dumps", "parse-traces", "front-diag-report"]

[front.profile."fast-dev"]
description = "Profil de développement rapide : se concentre sur vitte-front-check et les AST."
projects    = ["compiler-front-core", "compiler-front-scope"]
binaries    = ["vitte-front-check", "vitte-ast-dump"]
artifacts   = ["ast-dumps"]

[front.profile."ci"]
description = "Profil CI : exécute l'ensemble des vérifications front-end avec traces et rapports."
projects    = ["compiler-front-core", "compiler-front-scope", "compiler-front-tools"]
binaries    = ["vitte-front-check", "vitte-ast-dump", "vitte-parse-trace"]
artifacts   = ["ast-dumps", "parse-traces", "front-diag-report"]

# ---------------------------------------------------------------------------
# Lien logique avec la pipeline globale et les couches host/middle
# ---------------------------------------------------------------------------

[front.link.pipeline]
# Nom de la pipeline globale définie dans bootstrap/pipeline/mod.muf.
pipeline_name = "vitte-bootstrap"

# Phase de pipeline principalement incarnée par cette couche.
phases        = ["front-end"]

# ---------------------------------------------------------------------------
# Tooling hints – scripts et toolchain Vitte
# ---------------------------------------------------------------------------

[tool.vitte.front]
# Ordre recommandé pour la construction des composants front-end.
preferred_build_order = [
  "compiler-front-core",
  "compiler-front-scope",
  "compiler-front-tools",
]

# Programmes typiques utilisés par les scripts de bootstrap pour valider le front-end.
front_tools = [
  "vitte-front-check",
  "vitte-ast-dump",
  "vitte-parse-trace",
]

[tool.vitte.front.test]
# Attentes logiques pour les tests front-end.
expect_front_check_success  = true
expect_ast_dumps_present    = true
expect_parse_traces_present = true
