# ============================================================================
# Muffin manifest – Vitte bootstrap "back" layer
#
# Ce fichier décrit la couche "back" du bootstrap Vitte :
#   - composants du back-end du compilateur (IR, codegen, link, emission),
#   - projets logiques construits autour de ces composants,
#   - binaries/outils back-end (ir-dump, bytecode-emitter, link-check),
#   - artefacts intermédiaires produits (IR, bytecode, rapports de link).
#
# Il est purement déclaratif :
#   - aucune règle de build impérative,
#   - aucune dépendance explicite sur un langage autre que Vitte,
#   - uniquement des métadonnées pour la toolchain Vitte et les scripts.
# ============================================================================

[back]
name        = "vitte-bootstrap-back"
edition     = "2025"
description = "Back-end bootstrap layer for the Vitte compiler (IR/codegen/link/emission)."
version     = "0.1.0"

# Profil par défaut pour cette couche (dev/release/ci…)
default_profile = "dev"

# ---------------------------------------------------------------------------
# Rôles globaux de la couche back
# ---------------------------------------------------------------------------

[back.roles]
# Rôles principaux de la couche back :
#   - fournir un back-end cohérent pour le compilateur Vitte (IR, codegen, link),
#   - produire les artefacts IR/bytecode nécessaires au runtime/VM,
#   - supporter les outils de debug/validation (IR dumps, link reports).
roles = [
  "back-end-core",
  "ir-and-bytecode-provider",
  "link-and-emission-layer",
]

# ---------------------------------------------------------------------------
# Projets logiques de la couche back
# ---------------------------------------------------------------------------

[back.project."compiler-back-core"]
kind        = "library"
description = "Composants cœur du back-end Vitte (IR, codegen, link)."
root        = "src/vitte/compiler/back"
edition     = "2025"

modules = [
  "vitte.compiler.ir",
  "vitte.compiler.codegen.bytecode",
  "vitte.compiler.codegen.text",
  "vitte.compiler.link",
]

[back.project."compiler-back-tools"]
kind        = "tooling"
description = "Outils back-end (IR dumper, bytecode emitter, link inspector)."
root        = "src/vitte/compiler/back/tools"
edition     = "2025"

modules = [
  "vitte.compiler.back.ir_dump",
  "vitte.compiler.back.bytecode_emit",
  "vitte.compiler.back.link_inspect",
]

[back.project."compiler-back-passes"]
kind        = "library"
description = "Passes et transformations spécifiques au back-end (lowerings, passes de simplification)."
root        = "src/vitte/compiler/back/passes"
edition     = "2025"

modules = [
  "vitte.compiler.back.lowering",
  "vitte.compiler.back.simplify",
  "vitte.compiler.back.layout",
]

# ---------------------------------------------------------------------------
# Binaries/outils back-end
# ---------------------------------------------------------------------------

[back.binary."vitte-ir-dump"]
kind        = "program"
description = "Outil pour dumper l'IR Vitte (texte, graph, stats)."
root        = "src/vitte/compiler/back/tools"
entry       = "vitte.compiler.back.cli.ir_dump"
edition     = "2025"

requires_projects = [
  "compiler-back-core",
  "compiler-back-tools",
]

[back.binary."vitte-bytecode-emit"]
kind        = "program"
description = "Outil pour émettre du bytecode Vitte à partir de l'IR compilateur."
root        = "src/vitte/compiler/back/tools"
entry       = "vitte.compiler.back.cli.bytecode_emit"
edition     = "2025"

requires_projects = [
  "compiler-back-core",
  "compiler-back-passes",
  "compiler-back-tools",
]

[back.binary."vitte-link-check"]
kind        = "program"
description = "Outil pour vérifier et inspecter le link Vitte (résolution, symboles, sections)."
root        = "src/vitte/compiler/back/tools"
entry       = "vitte.compiler.back.cli.link_check"
edition     = "2025"

requires_projects = [
  "compiler-back-core",
  "compiler-back-tools",
]

# ---------------------------------------------------------------------------
# Artefacts back-end produits par cette couche
# ---------------------------------------------------------------------------

[back.artifact."ir-text-dumps"]
kind        = "collection"
format      = "vitte-ir-text"
description = "Dumps textuels de l'IR Vitte, utilisés pour le debug et les tests."
output_path = "target/bootstrap/back/ir"

[back.artifact."ir-graph-dumps"]
kind        = "collection"
format      = "vitte-ir-graph"
description = "Représentations graphiques/logiques de l'IR (graphes de contrôle/flux de données)."
output_path = "target/bootstrap/back/ir_graph"

[back.artifact."bytecode-output"]
kind        = "bundle"
format      = "vitte-bytecode"
description = "Bytecode Vitte généré par la couche back, consommé par le runtime/VM."
output_path = "target/bootstrap/back/bytecode"

[back.artifact."link-report"]
kind        = "report"
format      = "vitte-link-report"
description = "Rapports logiques issus de vitte-link-check (résolution, symboles, sections)."
output_path = "target/bootstrap/back/link/report.txt"

# ---------------------------------------------------------------------------
# Profils pour la couche back
# ---------------------------------------------------------------------------

[back.profile."dev"]
description = "Profil de développement back : tous les projets, binaries et artefacts back sont activés."
projects    = ["compiler-back-core", "compiler-back-tools", "compiler-back-passes"]
binaries    = ["vitte-ir-dump", "vitte-bytecode-emit", "vitte-link-check"]
artifacts   = ["ir-text-dumps", "ir-graph-dumps", "bytecode-output", "link-report"]

[back.profile."fast-dev"]
description = "Profil de développement rapide : se concentre sur l'émission de bytecode."
projects    = ["compiler-back-core", "compiler-back-passes"]
binaries    = ["vitte-bytecode-emit"]
artifacts   = ["bytecode-output"]

[back.profile."ci"]
description = "Profil CI back : vérifie l'IR, le bytecode et le link avec artefacts complets."
projects    = ["compiler-back-core", "compiler-back-tools", "compiler-back-passes"]
binaries    = ["vitte-ir-dump", "vitte-bytecode-emit", "vitte-link-check"]
artifacts   = ["ir-text-dumps", "ir-graph-dumps", "bytecode-output", "link-report"]

# ---------------------------------------------------------------------------
# Lien logique avec la pipeline globale et les autres couches bootstrap
# ---------------------------------------------------------------------------

[back.link.pipeline]
# Nom de la pipeline globale définie dans bootstrap/pipeline/mod.muf.
pipeline_name = "vitte-bootstrap"

# Phase de pipeline principalement incarnée par cette couche.
phases        = ["back-end"]

[back.link.middle]
# Référence à la couche middle (stage1/stage2) qui dépend fortement du back-end.
middle_name = "vitte-bootstrap-middle"

# Binaries middle alimentés par cette couche.
binaries    = [
  "vittec-stage1",
  "vittec-stage2",
]

# ---------------------------------------------------------------------------
# Tooling hints – scripts et toolchain Vitte
# ---------------------------------------------------------------------------

[tool.vitte.back]
# Ordre recommandé pour la construction des composants back-end.
preferred_build_order = [
  "compiler-back-core",
  "compiler-back-passes",
  "compiler-back-tools",
]

# Outils back-end typiques utilisés dans les scripts de bootstrap.
back_tools = [
  "vitte-ir-dump",
  "vitte-bytecode-emit",
  "vitte-link-check",
]

[tool.vitte.back.test]
# Attentes logiques pour les tests back-end.
expect_ir_dumps_present       = true
expect_bytecode_output_present = true
expect_link_report_present     = true
