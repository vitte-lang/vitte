# ============================================================================
# Muffin manifest – Vitte bootstrap "host" layer (stage0)
#
# Ce fichier décrit la couche "host" du bootstrap Vitte :
#   - outils et programmes exécutés directement sur la machine hôte,
#   - responsabilités du stage0 dans la chaîne de bootstrap,
#   - artefacts produits et mis à disposition pour les stages suivants,
#   - liens logiques avec la pipeline globale et la couche "middle".
#
# Il est purement déclaratif :
#   - aucune règle de build impérative,
#   - aucune dépendance codée ici sur un langage particulier,
#   - uniquement des métadonnées pour la toolchain Vitte et les scripts.
# ============================================================================

[host]
name        = "vitte-bootstrap-host"
edition     = "2025"
description = "Host-side bootstrap layer (stage0) for the Vitte toolchain."
version     = "0.1.0"

# Profil par défaut pour cette couche (dev/release/ci…)
default_profile = "dev"

# ---------------------------------------------------------------------------
# Rôles globaux de la couche host
# ---------------------------------------------------------------------------

[host.roles]
# Rôles principaux assurés par la couche host dans le bootstrap :
#   - fournir un ensemble minimal d'outils capables de construire
#     un premier compilateur Vitte utilisable,
#   - préparer et valider des artefacts initiaux (grammaires, tests, samples),
#   - orchestrer l'appel à la pipeline de compilation Vitte en mode "stage0".
roles = [
  "stage0-bootstrap",
  "host-tools-provider",
  "bootstrap-orchestrator",
  "test-harness-host",
]

# ---------------------------------------------------------------------------
# Projets logiques côté host
# ---------------------------------------------------------------------------

# Chaque bloc [host.project."…"] décrit un ensemble cohésif de fichiers,
# scripts, configurations ou outils hôtes, vus comme un "projet" logique.

[host.project."bootstrap-scripts"]
kind        = "scripts"
description = "Scripts et helpers hôtes pour piloter la pipeline de bootstrap."
root        = "scripts"
edition     = "2025"

# Scripts typiques (noms logiques, pas forcément exhaustifs).
scripts = [
  "bootstrap_stage0.sh",
  "self_host_stage1.sh",
  "clean.sh",
  "env_local.sh",
]

[host.project."spec-and-grammar"]
kind        = "metadata"
description = "Spécification du langage Vitte, grammaires et données associées."
root        = "grammar"
edition     = "2025"

# Fichiers logiques inclus dans ce projet.
files = [
  "grammar/vitte.pest",
  "grammar/vitte.ebnf",
  "docs/language-spec.md",
]

[host.project."test-fixtures"]
kind        = "tests"
description = "Fixtures de tests (samples, mini projects, données d'entrée) utilisées à stage0."
root        = "tests"
edition     = "2025"

fixtures = [
  "tests/data/mini_project",
  "tests/data/samples",
  "tests/data/lex",
  "tests/data/parse",
]

[host.project."host-tools-core"]
kind        = "host-tools"
description = "Outils hôtes génériques nécessaires au bootstrap (formatage, arbres, diagnostics)."
root        = "bootstrap/host/tools"
edition     = "2025"

tools = [
  "host.format-checker",
  "host.tree-dumper",
  "host.diag-reporter",
]

# ---------------------------------------------------------------------------
# Programmes logiques côté host (stage0)
# ---------------------------------------------------------------------------

# Ces "binaries" sont des vues logiques des exécutables utilisés à stage0.
# Le détail de leur construction (langage, build system) n’est PAS décrit ici.

[host.binary."vittec0"]
kind        = "host-program"
description = "Compiler de bootstrap initial utilisé pour construire le compilateur Vitte stage1."
root        = "bootstrap/host/vittec0"
entry       = "host.vittec0.main"
edition     = "2025"

requires_projects = [
  "spec-and-grammar",
  "test-fixtures",
]

[host.binary."host-test-runner"]
kind        = "host-program"
description = "Runner hôte pour exécuter les tests initiaux sur la grammaire et les samples."
root        = "bootstrap/host/test_runner"
entry       = "host.test_runner.main"
edition     = "2025"

requires_projects = [
  "test-fixtures",
  "bootstrap-scripts",
]

[host.binary."host-tools-bundle"]
kind        = "host-program"
description = "Bundle logique des petits outils hôtes utilisés dans les scripts de bootstrap."
root        = "bootstrap/host/tools"
entry       = "host.tools.main"
edition     = "2025"

requires_projects = [
  "host-tools-core",
]

# ---------------------------------------------------------------------------
# Artefacts produits par la couche host
# ---------------------------------------------------------------------------

[host.artifact."grammar-checked"]
kind        = "report"
format      = "vitte-grammar-report"
description = "Rapport logique sur la validité/consistance de la grammaire Vitte à stage0."
output_path = "target/bootstrap/host/grammar/report.txt"

[host.artifact."samples-validated"]
kind        = "report"
format      = "vitte-samples-report"
description = "Rapport logique sur les samples/fixtures compilés ou vérifiés à stage0."
output_path = "target/bootstrap/host/samples/report.txt"

[host.artifact."stage0-logs"]
kind        = "collection"
format      = "vitte-bootstrap-logs"
description = "Logs textuels de la phase host/stage0 du bootstrap."
output_path = "target/bootstrap/host/logs"

# ---------------------------------------------------------------------------
# Profils pour la couche host
# ---------------------------------------------------------------------------

[host.profile."dev"]
description = "Profil de développement host : tous les projets, binaries et artefacts host sont utilisés."
projects    = ["bootstrap-scripts", "spec-and-grammar", "test-fixtures", "host-tools-core"]
binaries    = ["vittec0", "host-test-runner", "host-tools-bundle"]
artifacts   = ["grammar-checked", "samples-validated", "stage0-logs"]

[host.profile."fast-dev"]
description = "Profil de développement rapide : se concentre sur vittec0 et la grammaire."
projects    = ["spec-and-grammar", "test-fixtures"]
binaries    = ["vittec0"]
artifacts   = ["grammar-checked"]

[host.profile."ci"]
description = "Profil CI : exécute l’ensemble des vérifications de stage0 et produit tous les artefacts."
projects    = ["bootstrap-scripts", "spec-and-grammar", "test-fixtures", "host-tools-core"]
binaries    = ["vittec0", "host-test-runner", "host-tools-bundle"]
artifacts   = ["grammar-checked", "samples-validated", "stage0-logs"]

# ---------------------------------------------------------------------------
# Lien logique avec la pipeline globale et la couche middle
# ---------------------------------------------------------------------------

[host.link.pipeline]
# Nom de la pipeline globale définie dans bootstrap/pipeline/mod.muf.
pipeline_name = "vitte-bootstrap"

# Phase principalement incarnée par cette couche dans la pipeline.
phases        = ["front-end", "tooling"]

# Stage(s) de bootstrap représentés par cette couche.
stages        = ["stage0-host-tools"]

[host.link.middle]
# Référence logique à la couche middle (stage1/stage2) décrite dans bootstrap/middle/mod.muf.
middle_name = "vitte-bootstrap-middle"

# Binaries middle directement dépendants des artefacts host.
binaries    = ["vittec-stage1", "vittec-stage2"]

# ---------------------------------------------------------------------------
# Tooling hints – scripts et toolchain Vitte
# ---------------------------------------------------------------------------

[tool.vitte.host]
# Ordre recommandé pour la préparation côté host.
preferred_init_order = [
  "bootstrap-scripts",
  "spec-and-grammar",
  "test-fixtures",
  "host-tools-core",
]

# Noms logiques de scripts hôtes typiques (exploités par la toolchain).
bootstrap_scripts = [
  "scripts/bootstrap_stage0.sh",
  "scripts/self_host_stage1.sh",
]

[tool.vitte.host.test]
# Attentes logiques pour les tests host/stage0.
expect_vittec0_success       = true
expect_grammar_report_present = true
expect_samples_report_present = true
