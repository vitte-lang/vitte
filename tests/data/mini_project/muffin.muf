

# ============================================================================
# Muffin manifest – mini_project
#
# Ce fichier décrit un mini projet de test utilisé par la suite de tests
# du compilateur Vitte. Il reste volontairement simple, mais suffisamment
# réaliste pour couvrir :
#   - un workspace logique,
#   - un projet applicatif avec point d’entrée `app.main`,
#   - une configuration d’édition et de toolchain Vitte.
# ============================================================================

[workspace]
name        = "mini_project"
edition     = "2025"
description = "Mini project used as smoke-test input for the Vitte toolchain."

# Racine du projet (relative à ce manifest)
root        = "."

# Liste des membres (ici un seul projet applicatif)
members     = ["mini_project_app"]

[project.mini_project_app]
kind        = "program"
name        = "mini_project_app"

# Répertoire source racine
root        = "src"

# Module d’entrée (cf. tests/data/mini_project/src/app/main.vitte)
entry       = "app.main"

# Edition du langage utilisée pour ce projet
edition     = "2025"

[tool.vitte]
# Contraintes minimum pour la toolchain Vitte attendue par ce sample
min_version = "0.1.0"
channel     = "dev"

# Options éventuelles pour les tests (purement décoratives ici)
[tool.vitte.test]
expect_build_success = true
expect_warnings      = false