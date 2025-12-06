# ============================================================================
# Makefile – vitte-core
#
# Objectifs :
#   - Fournir des cibles simples pour piloter le bootstrap Vitte (stage0, stage1+2),
#     le nettoyage, et quelques commandes de confort (env, mini_project, etc.).
#   - Rester entièrement centré sur le workspace Vitte (aucune référence à
#     d'autres langages/toolchains que Vitte).
#   - Servir de façade unique pour les scripts sous ./scripts/.
#
# Convention :
#   - Toutes les commandes shell passent par bash via /usr/bin/env.
#   - Les scripts clés sont :
#       * scripts/env_local.sh
#       * scripts/bootstrap_stage0.sh
#       * scripts/self_host_stage1.sh
#       * scripts/clean.sh
# ============================================================================

SHELL := /usr/bin/env bash

# Racine du workspace (ce dépôt).
WORKSPACE_ROOT := $(CURDIR)
SCRIPTS_DIR    := $(WORKSPACE_ROOT)/scripts
ENV_FILE       := $(SCRIPTS_DIR)/env_local.sh

TARGET_DIR     := $(WORKSPACE_ROOT)/target
MINI_PROJECT_MF := $(WORKSPACE_ROOT)/tests/data/mini_project/muffin.muf

# ---------------------------------------------------------------------------
# Helpers internes
# ---------------------------------------------------------------------------

# Préfixe pour exécuter un script dans un shell bash avec env_local.sh.
define VITTE_SHELL
if [ -f "$(ENV_FILE)" ]; then \
  source "$(ENV_FILE)"; \
fi; \
cd "$(WORKSPACE_ROOT)"; \
endef

# Texte d'aide : aligné avec les cibles ci-dessous.
HELP_TEXT = \
  "Cibles disponibles :" "\n" \
  "  help                 : Affiche cette aide." "\n" \
  "  env                  : Affiche l'environnement Vitte local (env_local.sh)." "\n" \
  "  bootstrap-stage0     : Lance le stage0 (host layer) du bootstrap Vitte." "\n" \
  "  bootstrap-stage1     : Lance les stages 1+2 (self-host compiler)." "\n" \
  "  bootstrap-all        : Enchaîne stage0 puis stage1+2." "\n" \
  "  mini_project         : Smoke build du mini projet de test via vittec." "\n" \
  "  clean                : Nettoie les artefacts générés (target/, fichiers temporaires)." "\n" \
  "  distclean            : Nettoyage plus agressif (supprime target/ entièrement)." "\n" \
  "  fmt                  : Placeholder pour formatage du code Vitte (à brancher plus tard)." "\n" \
  "  lint                 : Placeholder pour lint/validation Vitte (à brancher plus tard)." "\n" \
  "  test                 : Placeholder pour tests Vitte (à brancher plus tard)." "\n"

# ---------------------------------------------------------------------------
# Cible par défaut
# ---------------------------------------------------------------------------

.PHONY: default
default: help

# ---------------------------------------------------------------------------
# Aide
# ---------------------------------------------------------------------------

.PHONY: help
help:
	@printf $(HELP_TEXT)

# ---------------------------------------------------------------------------
# Environnement
# ---------------------------------------------------------------------------

.PHONY: env
env:
	@echo "[vitte][make] Workspace root : $(WORKSPACE_ROOT)"
	@echo "[vitte][make] Scripts dir    : $(SCRIPTS_DIR)"
	@if [ -f "$(ENV_FILE)" ]; then \
	  echo "[vitte][make] Sourcing env_local.sh…"; \
	  bash -lc "source '$(ENV_FILE)' >/dev/null 2>&1 || true; echo \"[vitte][env] VITTE_WORKSPACE_ROOT=\$${VITTE_WORKSPACE_ROOT}\"; echo \"[vitte][env] VITTE_BOOTSTRAP_ROOT=\$${VITTE_BOOTSTRAP_ROOT}\"; echo \"[vitte][env] VITTE_EDITION=\$${VITTE_EDITION}\"; echo \"[vitte][env] VITTE_PROFILE=\$${VITTE_PROFILE}\""; \
	else \
	  echo "[vitte][make] Aucun env_local.sh trouvé (scripts/env_local.sh)."; \
	fi

# ---------------------------------------------------------------------------
# Bootstrap – stage0 et stage1+2
# ---------------------------------------------------------------------------

.PHONY: bootstrap-stage0
bootstrap-stage0:
	@echo "[vitte][make] Running bootstrap stage0 (host layer)…"
	@bash -lc '$(VITTE_SHELL) ./scripts/bootstrap_stage0.sh'
	@echo "[vitte][make] bootstrap-stage0 terminé."

.PHONY: bootstrap-stage1
bootstrap-stage1:
	@echo "[vitte][make] Running bootstrap stage1+2 (self-host compiler)…"
	@bash -lc '$(VITTE_SHELL) ./scripts/self_host_stage1.sh'
	@echo "[vitte][make] bootstrap-stage1 terminé."

.PHONY: bootstrap-all
bootstrap-all: bootstrap-stage0 bootstrap-stage1
	@echo "[vitte][make] bootstrap-all terminé (stage0 + stage1+2)."

# ---------------------------------------------------------------------------
# Mini projet – smoke build
# ---------------------------------------------------------------------------

.PHONY: mini_project
mini_project:
	@echo "[vitte][make] Smoke build du mini_project via vittec (si présent)…"
	@if [ ! -f "$(MINI_PROJECT_MF)" ]; then \
	  echo "[vitte][make][WARN] Manifest mini_project introuvable : $(MINI_PROJECT_MF)"; \
	  exit 1; \
	fi
	@if [ ! -x "$(TARGET_DIR)/debug/vittec" ]; then \
	  echo "[vitte][make][WARN] target/debug/vittec introuvable ou non exécutable."; \
	  echo "[vitte][make][INFO] Tu peux d'abord lancer : make bootstrap-all"; \
	  exit 1; \
	fi
	@bash -lc '$(VITTE_SHELL) "$(TARGET_DIR)/debug/vittec" "$(MINI_PROJECT_MF)" || echo "[vitte][make][WARN] Commande vittec non encore implémentée ou en erreur (placeholder)."'

# ---------------------------------------------------------------------------
# Nettoyage
# ---------------------------------------------------------------------------

.PHONY: clean
clean:
	@echo "[vitte][make] Nettoyage du workspace via scripts/clean.sh…"
	@bash -lc '$(VITTE_SHELL) ./scripts/clean.sh'
	@echo "[vitte][make] clean terminé."

.PHONY: distclean
distclean: clean
	@echo "[vitte][make] Suppression complète du répertoire target/…"
	@if [ -d "$(TARGET_DIR)" ]; then \
	  rm -rf "$(TARGET_DIR)"; \
	  echo "[vitte][make] target/ supprimé."; \
	else \
	  echo "[vitte][make] Aucun target/ à supprimer."; \
	fi

# ---------------------------------------------------------------------------
# Placeholders pour formatting / lint / tests Vitte
# (à compléter lorsque les outils correspondants existeront)
# ---------------------------------------------------------------------------

.PHONY: fmt
fmt:
	@echo "[vitte][make] fmt : placeholder – à brancher sur vitte.tools.format quand disponible."

.PHONY: lint
lint:
	@echo "[vitte][make] lint : placeholder – à brancher sur des passes de validation Vitte."

.PHONY: test
test:
	@echo "[vitte][make] test : placeholder – à brancher sur vitte.tools.test_runner et les fixtures de tests."

# Fin du Makefile – vitte-core
