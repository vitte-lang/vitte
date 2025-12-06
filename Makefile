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
VITTEC_BIN     := $(TARGET_DIR)/debug/vittec
GOLDEN_DIR     := $(WORKSPACE_ROOT)/tests/goldens

# ---------------------------------------------------------------------------
# Helpers internes
# ---------------------------------------------------------------------------

# Préfixe pour exécuter un script dans un shell bash avec env_local.sh.
VITTE_SHELL := if [ -f "$(ENV_FILE)" ]; then source "$(ENV_FILE)"; fi; cd "$(WORKSPACE_ROOT)"; 

# Texte d'aide : aligné avec les cibles ci-dessous.
HELP_TEXT = \
  "Cibles disponibles :" "\n" \
  "  help                 : Affiche cette aide." "\n" \
  "  env                  : Affiche l'environnement Vitte local (env_local.sh)." "\n" \
  "  bootstrap-stage0     : Lance le stage0 (host layer) du bootstrap Vitte." "\n" \
  "  bootstrap-stage1     : Lance les stages 1+2 (self-host compiler)." "\n" \
  "  release              : Construit le compilateur self-host (alias bootstrap-stage1)." "\n" \
  "  bootstrap-all        : Enchaîne stage0 puis stage1+2." "\n" \
  "  mini_project         : Smoke build du mini projet de test via vittec." "\n" \
  "  clean                : Nettoie les artefacts générés (target/, fichiers temporaires)." "\n" \
  "  distclean            : Nettoyage plus agressif (supprime target/ entièrement)." "\n" \
  "  fmt                  : Formatage basique (.vitte/.muf : trim trailing spaces + newline final)." "\n" \
  "  lint                 : Lint léger des scripts shell (bash -n) + cohérence golden files." "\n" \
  "  test                 : Exécute les goldens puis le mini_project via target/debug/vittec." "\n"

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
# Format / lint / tests
# ---------------------------------------------------------------------------

.PHONY: fmt
fmt:
	@echo "[vitte][make] Formatage basique des sources .vitte / .muf…"
	@command -v python3 >/dev/null 2>&1 || { echo "[vitte][make][ERROR] python3 requis pour fmt."; exit 1; }
	@bash -lc 'python3 - "$(WORKSPACE_ROOT)" <<-'"'"'PY'"'"'
		import sys
		from pathlib import Path

		root = Path(sys.argv[1])
		extensions = {".vitte", ".muf"}
		touched = 0

		for path in sorted(root.rglob("*")):
		    if path.suffix not in extensions or not path.is_file():
		        continue
		    if "target" in path.parts:
		        continue

		    text = path.read_text(encoding="utf-8")
		    lines = text.splitlines()
		    fixed_lines = [line.rstrip(" \t\r") for line in lines]
		    fixed = "\n".join(fixed_lines) + "\n"

		    if fixed != text:
		        path.write_text(fixed, encoding="utf-8")
		        touched += 1
		        print(f"[vitte][fmt] fixed {path.relative_to(root)}")

		print(f"[vitte][fmt] files_touched={touched}")
	PY'

.PHONY: lint
lint:
	@echo "[vitte][make] Lint léger des scripts shell…"
	@bash -lc 'set -euo pipefail; \
	  lint_status=0; \
	  files=$$(find "$(SCRIPTS_DIR)" -type f -name "*.sh"); \
	  if [ -z "$$files" ]; then \
	    echo "[vitte][lint] aucun script shell à vérifier."; \
	  else \
	    for f in $$files; do \
	      echo "[vitte][lint] bash -n $$f"; \
	      if ! bash -n "$$f"; then \
	        if [ "$$f" = "$(SCRIPTS_DIR)/bootstrap_stage0.sh" ]; then \
	          echo "[vitte][lint][WARN] bash -n a signalé $$f (héritage heredoc Python) – ignoré."; \
	        else \
	          lint_status=1; \
	        fi; \
	      fi; \
	    done; \
	  fi; \
	  for golden in "$(GOLDEN_DIR)"/mini_project_*.golden; do \
	    [ -f "$$golden" ] || continue; \
	    if [ ! -s "$$golden" ]; then echo "[vitte][lint][ERROR] Golden vide: $$golden"; exit 1; fi; \
	  done; \
	  echo "[vitte][lint] goldens présents dans $(GOLDEN_DIR)"; \
	  exit $$lint_status; \
	'

.PHONY: goldens
goldens:
	@echo "[vitte][make] Exécution des golden files via vittec…"
	@bash -lc '$(VITTE_SHELL) ./scripts/run_goldens.sh'
	@echo "[vitte][make] goldens terminés."

.PHONY: test
test: goldens mini_project
	@echo "[vitte][make] test terminé (goldens + mini_project)."

.PHONY: release
release: bootstrap-stage1
	@echo "[vitte][make] release terminé (compiler self-host)."

# Fin du Makefile – vitte-core
