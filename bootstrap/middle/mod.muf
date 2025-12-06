# ============================================================================
# Muffin manifest – Vitte bootstrap "middle" layer
#
# Ce fichier décrit la couche intermédiaire ("middle") de bootstrap :
#   - comment le compilateur Vitte est découpé en sous-composants logiques,
#   - quels modules doivent être disponibles en Vitte pur à ce stade,
#   - quels artefacts intermédiaires sont produits et réutilisés,
#   - comment cette couche s’insère entre les stages de bootstrap.
#
# Il est purement déclaratif :
#   - aucune référence à d’autres langages que Vitte,
#   - aucune règle de build impérative,
#   - uniquement des métadonnées pour la toolchain et les scripts.
# ============================================================================

[middle]
name        = "vitte-bootstrap-middle"
edition     = "2025"
description = "Intermediate bootstrap layer for the Vitte compiler (stage1/stage2)."
version     = "0.1.0"

# Profil par défaut pour cette couche (dev/release/ci…)
default_profile = "dev"

# ---------------------------------------------------------------------------
# Vue d’ensemble : rôles de la couche middle
# ---------------------------------------------------------------------------

[middle.roles]
# Rôles principaux assurés par cette couche du bootstrap :
#   - disposer d’un compilateur Vitte “suffisant” pour compiler lui-même
#     une grande partie de `vitte-core`,
#   - produire des artefacts intermédiaires (IR, bytecode, std) partageables,
#   - servir de pivot entre :
#       * stage0 (tooling minimal, compilateurs externes),
#       * stage2 (compilateur pleinement auto-hébergé).
roles = [
  "hosted-compiler-stage1",
  "self-compiler-stage2",
  "shared-artifacts-provider",
]

# ---------------------------------------------------------------------------
# Projets logiques de la couche middle
# ---------------------------------------------------------------------------

# Chaque bloc [middle.project."…"] décrit un "gros morceau" logique de la couche,
# généralement mappé sur une partie de l’arborescence `src/` de vitte-core.

[middle.project."compiler-front"]
kind        = "library"
description = "Front-end Vitte (lexer, parser, AST, spans, diagnostics, scope, symbols)."
root        = "src/vitte/compiler/front"
edition     = "2025"

modules = [
  "vitte.compiler.lexer",
  "vitte.compiler.parser",
  "vitte.compiler.ast",
  "vitte.compiler.span",
  "vitte.compiler.diagnostics",
  "vitte.compiler.scope",
  "vitte.compiler.symbols",
]

[middle.project."compiler-middle"]
kind        = "library"
description = "Middle-end Vitte (types, HIR, MIR, typecheck, analyses, optimisations simples)."
root        = "src/vitte/compiler/middle"
edition     = "2025"

modules = [
  "vitte.compiler.types",
  "vitte.compiler.hir",
  "vitte.compiler.mir",
  "vitte.compiler.typecheck",
  "vitte.compiler.constant_fold",
  "vitte.compiler.flow_analysis",
  "vitte.compiler.optimize",
]

[middle.project."compiler-back"]
kind        = "library"
description = "Back-end Vitte (IR canonique, génération de bytecode, IR texte)."
root        = "src/vitte/compiler/back"
edition     = "2025"

modules = [
  "vitte.compiler.ir",
  "vitte.compiler.codegen.bytecode",
  "vitte.compiler.codegen.text",
  "vitte.compiler.link",
]

[middle.project."runtime-core"]
kind        = "library"
description = "Cœur du runtime/VM Vitte nécessaire au bootstrap (bytecode, VM, hooks std)."
root        = "src/vitte/runtime"
edition     = "2025"

modules = [
  "vitte.runtime.vm",
  "vitte.runtime.bytecode",
  "vitte.runtime.gc",
  "vitte.runtime.std_hooks",
]

[middle.project."tools-core"]
kind        = "tooling"
description = "Outils de base utilisés pendant le middle bootstrap (fmt, symbols, tests)."
root        = "src/vitte/tools"
edition     = "2025"

modules = [
  "vitte.tools.format",
  "vitte.tools.symbols",
  "vitte.tools.test_runner",
]

# ---------------------------------------------------------------------------
# Binaries/logical programs produits à ce stade
# ---------------------------------------------------------------------------

# Ces "programs" sont des vues logiques : la manière exacte dont ils sont
# construits est du ressort des scripts de build, pas de ce manifest.

[middle.binary."vittec-stage1"]
kind        = "program"
description = "First Vitte compiler built mostly in Vitte (stage1)."
root        = "src/vitte/compiler"
entry       = "vitte.compiler.cli.main"
edition     = "2025"

requires_projects = [
  "compiler-front",
  "compiler-middle",
  "compiler-back",
  "runtime-core",
]

[middle.binary."vittec-stage2"]
kind        = "program"
description = "Self-hosted Vitte compiler rebuilt by vittec-stage1 (stage2)."
root        = "src/vitte/compiler"
entry       = "vitte.compiler.cli.main"
edition     = "2025"

requires_projects = [
  "compiler-front",
  "compiler-middle",
  "compiler-back",
  "runtime-core",
  "tools-core",
]

[middle.binary."vitte-run"]
kind        = "program"
description = "Runner pour exécuter du bytecode Vitte (utilisé par les tests et le bootstrap)."
root        = "src/vitte/runtime"
entry       = "vitte.runtime.cli.run"
edition     = "2025"

requires_projects = [
  "runtime-core",
]

# ---------------------------------------------------------------------------
# Artefacts intermédiaires produits par la couche middle
# ---------------------------------------------------------------------------

# Ce bloc décrit les "outputs" logiques de la couche middle, utilisés ensuite
# par d’autres stages, mais ne prescrit pas comment ils sont construits.

[middle.artifact."std-bundle"]
kind        = "bundle"
format      = "vitte-std-bundle"
description = "Bundle de la bibliothèque standard Vitte (std) compilée pour le runtime."

# Emplacement logique (relatif à la racine du repo / du bootstrap).
output_path = "target/bootstrap/std/std-bundle.vitte"

[middle.artifact."bytecode-tests"]
kind        = "collection"
format      = "vitte-bytecode-tests"
description = "Ensemble de bytecodes générés depuis des tests Vitte (unitaires + samples)."

output_path = "target/bootstrap/tests/bytecode"

[middle.artifact."compiler-ir-dumps"]
kind        = "collection"
format      = "vitte-ir-text"
description = "Dumps textuels IR/HIR/MIR utilisés pour les tests et la validation."

output_path = "target/bootstrap/ir-dumps"

# ---------------------------------------------------------------------------
# Profils pour la couche middle
# ---------------------------------------------------------------------------

[middle.profile."dev"]
description = "Profil de développement : tous les binaries et artefacts middle sont construits."
projects    = ["compiler-front", "compiler-middle", "compiler-back", "runtime-core", "tools-core"]
binaries    = ["vittec-stage1", "vittec-stage2", "vitte-run"]
artifacts   = ["std-bundle", "bytecode-tests", "compiler-ir-dumps"]

[middle.profile."fast-dev"]
description = "Profil de développement rapide : seul vittec-stage1 est construit, artefacts limités."
projects    = ["compiler-front", "compiler-middle", "compiler-back", "runtime-core"]
binaries    = ["vittec-stage1"]
artifacts   = ["compiler-ir-dumps"]

[middle.profile."ci"]
description = "Profil CI : builds complets + génération d’artefacts pour tests et validation."
projects    = ["compiler-front", "compiler-middle", "compiler-back", "runtime-core", "tools-core"]
binaries    = ["vittec-stage1", "vittec-stage2", "vitte-run"]
artifacts   = ["std-bundle", "bytecode-tests", "compiler-ir-dumps"]

# ---------------------------------------------------------------------------
# Lien logique avec la pipeline globale (bootstrap/pipeline/mod.muf)
# ---------------------------------------------------------------------------

[middle.link.pipeline]
# Nom logique de la pipeline à laquelle cette couche est rattachée.
pipeline_name = "vitte-bootstrap"

# Phases principalement concernées par cette couche.
phases        = ["front-end", "middle-end", "back-end", "runtime"]

# Stage(s) de bootstrap principalement alimentés par cette couche.
stages        = ["stage1-bootstrap-compiler", "stage2-self-hosted-compiler"]

# ---------------------------------------------------------------------------
# Tooling hints – scripts et toolchain Vitte
# ---------------------------------------------------------------------------

[tool.vitte.middle]
# Ce bloc est un espace réservé pour la toolchain Vitte afin d’exprimer
# des préférences pour l’ordonnancement de la couche middle dans les scripts.
#
# Les clés sont purement indicatives pour les scripts shell / runners
# (aucun impact sémantique sur le langage Vitte).
preferred_build_order = [
  "compiler-front",
  "compiler-middle",
  "compiler-back",
  "runtime-core",
  "tools-core",
]

# Scripts ou commandes typiques pouvant utiliser ces infos (noms logiques).
bootstrap_scripts = [
  "scripts/bootstrap_stage0.sh",
  "scripts/self_host_stage1.sh",
]

[tool.vitte.middle.test]
# Attentes logiques pour les tests de la couche middle.
expect_vittec_stage1_success = true
expect_vittec_stage2_success = true
expect_std_bundle_present    = true
