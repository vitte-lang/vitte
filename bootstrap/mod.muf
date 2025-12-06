# ============================================================================
# Muffin manifest – Vitte bootstrap workspace (bootstrap/)
#
# Ce fichier décrit la vue d’ensemble du bootstrap Vitte au niveau du dossier
# `bootstrap/` du workspace `vitte-core` :
#   - agrégation des différentes couches (host, front, back, middle, cli, core, pipeline),
#   - profils globaux de bootstrap (dev, fast-dev, ci),
#   - liens vers le workspace racine et la std,
#   - hints pour la toolchain Vitte et les scripts.
#
# Il est purement déclaratif :
#   - aucune règle de build impérative,
#   - aucune référence à d’autres langages que Vitte,
#   - uniquement des métadonnées de structure pour le bootstrap.
# ============================================================================

[bootstrap]
name        = "vitte-bootstrap-workspace"
edition     = "2025"
description = "Global bootstrap workspace view for the Vitte toolchain."
version     = "0.1.0"

# Profil par défaut de bootstrap (peut être redéfini par les scripts/CI).
default_profile = "dev"

# ---------------------------------------------------------------------------
# Vue workspace : racine, manifests et std
# ---------------------------------------------------------------------------

[bootstrap.workspace]
# Racine logique du workspace vitte-core (relative à ce manifest).
root = ".."

# Manifest global du workspace (racine du repo).
root_manifest = "muffin.muf"

# Manifest de la bibliothèque standard.
std_manifest = "src/std/mod.muf"

# Manifests bootstrap spécialisés (couches).
layer_manifests = [
  "bootstrap/host/mod.muf",
  "bootstrap/front/mod.muf",
  "bootstrap/middle/mod.muf",
  "bootstrap/back/mod.muf",
  "bootstrap/cli/mod.muf",
  "bootstrap/core/mod.muf",
  "bootstrap/pipeline/mod.muf",
]

# ---------------------------------------------------------------------------
# Couches de bootstrap (host, front, back, middle, cli, core, pipeline)
# ---------------------------------------------------------------------------

[bootstrap.layer."host"]
kind        = "bootstrap-layer"
description = "Host/stage0 layer – scripts, vittec0, fixtures, host tools."
manifest    = "bootstrap/host/mod.muf"
logical_name = "vitte-bootstrap-host"

[bootstrap.layer."front"]
kind        = "bootstrap-layer"
description = "Front-end layer – lexer, parser, AST, diag, front tools."
manifest    = "bootstrap/front/mod.muf"
logical_name = "vitte-bootstrap-front"

[bootstrap.layer."middle"]
kind        = "bootstrap-layer"
description = "Middle layer – compiler front/middle/back, runtime core, tools."
manifest    = "bootstrap/middle/mod.muf"
logical_name = "vitte-bootstrap-middle"

[bootstrap.layer."back"]
kind        = "bootstrap-layer"
description = "Back-end layer – IR, codegen, link, emission tools."
manifest    = "bootstrap/back/mod.muf"
logical_name = "vitte-bootstrap-back"

[bootstrap.layer."cli"]
kind        = "bootstrap-layer"
description = "CLI layer – vittec, vitte-run, vitte-tools, vitte-bootstrap."
manifest    = "bootstrap/cli/mod.muf"
logical_name = "vitte-bootstrap-cli"

[bootstrap.layer."core"]
kind        = "bootstrap-layer"
description = "Core layer – global view of compiler/runtime/std/tools and bootstrap components."
manifest    = "bootstrap/core/mod.muf"
logical_name = "vitte-bootstrap-core"

[bootstrap.layer."pipeline"]
kind        = "bootstrap-layer"
description = "Pipeline layer – logical compilation pipeline (phases, artifacts, stages)."
manifest    = "bootstrap/pipeline/mod.muf"
logical_name = "vitte-bootstrap"

# ---------------------------------------------------------------------------
# Rôles globaux de ce bootstrap workspace
# ---------------------------------------------------------------------------

[bootstrap.roles]
# Rôles principaux de ce manifest d’agrégation :
#   - servir de point d’entrée unique pour la configuration bootstrap,
#   - faciliter la navigation tooling (LSP, indexer, visualiseurs),
#   - décrire quels sous-manifests composent le bootstrap complet.
roles = [
  "bootstrap-root",
  "bootstrap-configuration-aggregator",
  "bootstrap-toolchain-entry",
]

# ---------------------------------------------------------------------------
# Profils globaux de bootstrap
# ---------------------------------------------------------------------------

# Profil de développement complet.
[bootstrap.profile."dev"]
description = "Profil de développement bootstrap : toutes les couches sont actives."
layers      = ["host", "front", "middle", "back", "cli", "core", "pipeline"]

# Profil de développement rapide – se concentre sur le cœur compiler/runtime/CLI.
[bootstrap.profile."fast-dev"]
description = "Profil de développement rapide : focus sur compiler/runtime/CLI, host minimal."
layers      = ["host", "front", "middle", "back", "cli", "core"]

# Profil CI – exécute tout le bootstrap + pipeline complet.
[bootstrap.profile."ci"]
description = "Profil CI bootstrap : toutes les couches + pipeline sont prises en compte."
layers      = ["host", "front", "middle", "back", "cli", "core", "pipeline"]

# ---------------------------------------------------------------------------
# Lien avec la pipeline globale (bootstrap/pipeline/mod.muf)
# ---------------------------------------------------------------------------

[bootstrap.link.pipeline]
pipeline_manifest = "bootstrap/pipeline/mod.muf"
pipeline_name     = "vitte-bootstrap"

# Phases connues dans la pipeline.
phases = [
  "front-end",
  "middle-end",
  "back-end",
  "runtime",
  "tooling",
]

# ---------------------------------------------------------------------------
# Références rapides vers les principaux binaries logiques de bootstrap
# (tels que vus dans les manifests de couches correspondants)
# ---------------------------------------------------------------------------

[bootstrap.binaries]
compiler_stage0   = "vittec0"                # host layer
compiler_stage1   = "vittec-stage1"          # middle layer
compiler_stage2   = "vittec-stage2"          # middle layer
compiler_main     = "vittec"                 # core/cli layers
runner_main       = "vitte-run"              # core/cli layers
tools_bundle      = "vitte-tools"            # core/cli layers
bootstrap_command = "vitte-bootstrap"        # cli layer

# ---------------------------------------------------------------------------
# Tooling hints – pour scripts, toolchain et IDE
# ---------------------------------------------------------------------------

[tool.vitte.bootstrap_root]
# Ordre recommandé pour l’initialisation du bootstrap complet.
preferred_init_layers = [
  "host",
  "front",
  "middle",
  "back",
  "cli",
  "core",
  "pipeline",
]

# Références aux manifests pour les outils qui veulent charger tout le bootstrap.
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

[tool.vitte.bootstrap_root.test]
# Attentes logiques de haut niveau pour les tests d’intégration de bootstrap.
expect_stage0_success = true
expect_stage1_success = true
expect_stage2_success = true
expect_core_binaries_present = true
