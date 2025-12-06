

# ============================================================================
# Muffin manifest – std library index
#
# Ce fichier décrit la bibliothèque standard de Vitte ("std") au sens logique.
# Il ne contient aucun code exécutable : uniquement des métadonnées déclaratives
# utilisées par :
#   - le compilateur Vitte,
#   - la toolchain (résolution de modules std),
#   - les outils (LSP, formateur, indexer, etc.).
#
# Objectifs :
#   - exposer les modules std avec leurs chemins sources,
#   - indiquer leur stabilité (stable / experimental),
#   - déclarer des "features" activables (par édition / profil),
#   - rester lisible et facile à étendre.
# ============================================================================

[std]
name        = "std"
description = "Standard library for the Vitte language."
edition     = "2025"

# Version logique du bundle std actuellement décrit.
# Cette version ne reflète pas forcément une version de binaire, mais un schéma.
version     = "0.1.0"

# Profil par défaut (debug/release/host…) si nécessaire.
default_profile = "dev"

# Liste des modules racine exposés par la bibliothèque standard.
modules = [
  "std.collections",
  "std.fs",
  "std.io",
  "std.path",
  "std.string",
  "std.time",
]

# ---------------------------------------------------------------------------
# Modules : std.collections
# ---------------------------------------------------------------------------

[std.module."std.collections"]
path        = "src/std/collections"
description = "Core collections for Vitte (Vec, HashMap, registries, etc.)."
stability   = "unstable"   # encore en évolution
edition     = "2025"

# Sous-modules déclarés à titre documentaire (non normatif).
submodules = [
  "std.collections.std_vec",
  "std.collections.std_map",
]

[std.module."std.collections.feature.default"]
description = "Default collections set (Vec, HashMap, registries…)."
enabled     = true

# ---------------------------------------------------------------------------
# Modules : std.fs
# ---------------------------------------------------------------------------

[std.module."std.fs"]
path        = "src/std/fs"
description = "Logical filesystem layer and models for Vitte."
stability   = "unstable"
edition     = "2025"

submodules = [
  "std.fs.std_fs",
]

[std.module."std.fs.feature.default"]
description = "Expose filesystem logical models (paths, handles, ops, registry)."
enabled     = true

# ---------------------------------------------------------------------------
# Modules : std.io
# ---------------------------------------------------------------------------

[std.module."std.io"]
path        = "src/std/io"
description = "Logical I/O models (streams, buffers, read_to_string, etc.)."
stability   = "unstable"
edition     = "2025"

submodules = [
  "std.io.std_io",
  "std.io.read_to_string",
]

[std.module."std.io.feature.default"]
description = "Enable basic I/O models and text read helpers."
enabled     = true

# ---------------------------------------------------------------------------
# Modules : std.path
# ---------------------------------------------------------------------------

[std.module."std.path"]
path        = "src/std/path"
description = "Logical path handling (domains, styles, normalization, registry)."
stability   = "unstable"
edition     = "2025"

submodules = [
  "std.path.std_path",
]

[std.module."std.path.feature.default"]
description = "Enable generic path models for filesystem, URLs, modules, packages."
enabled     = true

# ---------------------------------------------------------------------------
# Modules : std.string
# ---------------------------------------------------------------------------

[std.module."std.string"]
path        = "src/std/string"
description = "String core models (owned, slices, ropes, interner, ops)."
stability   = "unstable"
edition     = "2025"

submodules = [
  "std.string.std_string",
]

[std.module."std.string.feature.default"]
description = "Enable core string models, interning and string-ops contracts."
enabled     = true

# ---------------------------------------------------------------------------
# Modules : std.time
# ---------------------------------------------------------------------------

[std.module."std.time"]
path        = "src/std/time"
description = "Time models (instants, durations, clocks, timers, zones, registry)."
stability   = "unstable"
edition     = "2025"

submodules = [
  "std.time.std_time",
]

[std.module."std.time.feature.default"]
description = "Enable time/clock/duration models for the Vitte ecosystem."
enabled     = true

# ---------------------------------------------------------------------------
# Profils / éditions
# ---------------------------------------------------------------------------

[std.profile."dev"]
description = "Development profile for std: all modules + experimental pieces."
edition     = "2025"
features    = ["std.collections.feature.default",
               "std.fs.feature.default",
               "std.io.feature.default",
               "std.path.feature.default",
               "std.string.feature.default",
               "std.time.feature.default"]

[std.profile."release"]
description = "Release profile for std: same modules, but may restrict experimental parts."
edition     = "2025"
features    = ["std.collections.feature.default",
               "std.fs.feature.default",
               "std.io.feature.default",
               "std.path.feature.default",
               "std.string.feature.default",
               "std.time.feature.default"]

# ---------------------------------------------------------------------------
# Tooling hints – purement déclaratif
# ---------------------------------------------------------------------------

[tool.vitte.std]
# Ce bloc est un espace réservé pour la toolchain Vitte afin d’exprimer
# des préférences de chargement de la std, sans impact sur la sémantique
# du langage lui-même.
resolve_mode        = "from-workspace"   # ou "from-prebuilt-bundle"
allow_partial_std   = true              # utile pour les tests de bootstrap
allow_internal_apis = true              # expose temporairement des détails internes

[tool.vitte.std.test]
# Paramètres indicatifs pour les tests qui utilisent cette std.
expect_load_success = true
expect_warnings     = false