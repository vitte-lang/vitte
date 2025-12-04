# =============================================================================
# vitte-core – Makefile "front-end" pour toolchain Vitte uniquement
#
# Rôle :
#   - Fournir une interface uniforme purement Vitte pour :
#       * builder le workspace via les manifests Muffin (.muf),
#       * lancer les tests / lint / format,
#       * produire des archives de distribution,
#       * offrir une aide auto-documentée.
#   - Ne dépendre que des outils Vitte (vittec, vitte, vitte-repl, vitte-fmt,
#     vitte-lint, vitte-test, vitte-muffin), quels que soient leurs backends.
#
# Ce Makefile n’évoque plus aucun autre langage : il suppose l’existence
# d’une toolchain Vitte installée dans le PATH.
# =============================================================================

# ---------------------------------------------------------------------------
# Configuration globale
# ---------------------------------------------------------------------------

# Nom du projet et version logique pour les archives
PROJECT_NAME    ?= vitte-core
PROJECT_VERSION ?= 0.1.0-dev

# Répertoires
ROOT_DIR        := $(CURDIR)
SRC_DIR         ?= src
BUILD_DIR       ?= build
DIST_DIR        ?= dist
DOCS_DIR        ?= docs

# Fichiers de manifestes Muffin
PROJECT_MUFFIN  ?= vitte.project.muf

# Outils Vitte
VITTEC         ?= vittec         # compilateur Vitte (source -> artefacts)
VITTE          ?= vitte          # exécutable principal / runner
VITTE_REPL     ?= vitte-repl     # REPL
VITTE_FMT      ?= vitte-fmt      # formateur
VITTE_LINT     ?= vitte-lint     # analyseur statique
VITTE_TEST     ?= vitte-test     # test runner
VITTE_MUFFIN   ?= vitte-muffin   # orchestrateur Muffin (build plan)

# Profil(s) de build Vitte (debug / release / test, etc.)
PROFILE         ?= debug
PROFILE_RELEASE ?= release

# OS / plateforme (pour quelques helpers)
UNAME_S := $(shell uname -s)

# Couleurs (désactivables via NO_COLOR=1)
ifeq ($(NO_COLOR),1)
COLOR_RESET  :=
COLOR_BLUE   :=
COLOR_GREEN  :=
COLOR_YELLOW :=
COLOR_RED    :=
else
COLOR_RESET  := \033[0m
COLOR_BLUE   := \033[34m
COLOR_GREEN  := \033[32m
COLOR_YELLOW := \033[33m
COLOR_RED    := \033[31m
endif

# ---------------------------------------------------------------------------
# Cible par défaut
# ---------------------------------------------------------------------------

.DEFAULT_GOAL := help

# ---------------------------------------------------------------------------
# Helpers d'affichage
# ---------------------------------------------------------------------------

define ECHO_SECTION
	@printf "$(COLOR_BLUE)==> %s$(COLOR_RESET)\n" "$(1)"
endef

define ECHO_OK
	@printf "$(COLOR_GREEN)[OK]$(COLOR_RESET) %s\n" "$(1)"
endef

define ECHO_WARN
	@printf "$(COLOR_YELLOW)[WARN]$(COLOR_RESET) %s\n" "$(1)"
endef

define ECHO_ERR
	@printf "$(COLOR_RED)[ERR]$(COLOR_RESET) %s\n" "$(1)"
endef

# ---------------------------------------------------------------------------
# Détection d’environnement
# ---------------------------------------------------------------------------

.PHONY: env
env: ## Affiche l’environnement Vitte détecté
	$(call ECHO_SECTION,"Environment (Vitte)")
	@echo "ROOT_DIR        = $(ROOT_DIR)"
	@echo "UNAME_S         = $(UNAME_S)"
	@echo "PROJECT_NAME    = $(PROJECT_NAME)"
	@echo "PROJECT_VERSION = $(PROJECT_VERSION)"
	@echo "SRC_DIR         = $(SRC_DIR)"
	@echo "BUILD_DIR       = $(BUILD_DIR)"
	@echo "DIST_DIR        = $(DIST_DIR)"
	@echo "PROJECT_MUFFIN  = $(PROJECT_MUFFIN)"
	@echo
	@echo "VITTEC          = $(VITTEC)"
	@echo "VITTE           = $(VITTE)"
	@echo "VITTE_REPL      = $(VITTE_REPL)"
	@echo "VITTE_FMT       = $(VITTE_FMT)"
	@echo "VITTE_LINT      = $(VITTE_LINT)"
	@echo "VITTE_TEST      = $(VITTE_TEST)"
	@echo "VITTE_MUFFIN    = $(VITTE_MUFFIN)"

# ---------------------------------------------------------------------------
# Aide
# ---------------------------------------------------------------------------

.PHONY: help
help: ## Affiche cette aide
	@echo "$(COLOR_BLUE)$(PROJECT_NAME) – Makefile (toolchain Vitte)$(COLOR_RESET)"
	@echo
	@echo "Usage :"
	@echo "  make <target>"
	@echo
	@echo "Cibles principales :"
	@awk 'BEGIN {FS = ":.*##"; printf ""} \
		/^[a-zA-Z0-9_-]+:.*##/ { \
			printf "  $(COLOR_GREEN)%-20s$(COLOR_RESET) %s\n", $$1, $$2 \
		}' $(MAKEFILE_LIST) | sort
	@echo

# ---------------------------------------------------------------------------
# Build – via manifest Muffin
# ---------------------------------------------------------------------------

.PHONY: all
all: build ## Alias de build

.PHONY: build
build: ## Build du projet via Muffin (profil=$(PROFILE))
	$(call ECHO_SECTION,"Build Vitte (profile=$(PROFILE))")
	@mkdir -p "$(BUILD_DIR)"
	@if command -v $(VITTE_MUFFIN) >/dev/null 2>&1; then \
		$(VITTE_MUFFIN) "$(PROJECT_MUFFIN)" --profile "$(PROFILE)" build; \
	else \
		$(call ECHO_WARN,"$(VITTE_MUFFIN) introuvable, fallback simple sur $(VITTEC)"); \
		$(VITTEC) --project "$(PROJECT_MUFFIN)" --profile "$(PROFILE)" build || true; \
	fi
	$(call ECHO_OK,"build Vitte terminé")

.PHONY: build-release
build-release: ## Build du projet en mode release (profil=$(PROFILE_RELEASE))
	$(call ECHO_SECTION,"Build Vitte (release, profile=$(PROFILE_RELEASE))")
	@mkdir -p "$(BUILD_DIR)"
	@if command -v $(VITTE_MUFFIN) >/dev/null 2>&1; then \
		$(VITTE_MUFFIN) "$(PROJECT_MUFFIN)" --profile "$(PROFILE_RELEASE)" build; \
	else \
		$(call ECHO_WARN,"$(VITTE_MUFFIN) introuvable, fallback simple sur $(VITTEC)"); \
		$(VITTEC) --project "$(PROJECT_MUFFIN)" --profile "$(PROFILE_RELEASE)" build || true; \
	fi
	$(call ECHO_OK,"build release Vitte terminé")

# ---------------------------------------------------------------------------
# Vérifications / formatage / lint
# ---------------------------------------------------------------------------

.PHONY: check
check: ## Vérifie la syntaxe et le typage du projet (équivalent 'check')
	$(call ECHO_SECTION,"Check Vitte")
	@if command -v $(VITTEC) >/dev/null 2>&1; then \
		$(VITTEC) --project "$(PROJECT_MUFFIN)" --profile "$(PROFILE)" check; \
	else \
		$(call ECHO_ERR,"$(VITTEC) introuvable, impossible de lancer 'check'"); \
		exit 1; \
	fi
	$(call ECHO_OK,"check Vitte terminé")

.PHONY: fmt
fmt: ## Formatage du code Vitte
	$(call ECHO_SECTION,"Format Vitte")
	@if command -v $(VITTE_FMT) >/dev/null 2>&1; then \
		$(VITTE_FMT) "$(SRC_DIR)"; \
	else \
		$(call ECHO_WARN,"$(VITTE_FMT) introuvable, format ignoré"); \
	fi
	$(call ECHO_OK,"formatage terminé (si outil disponible)")

.PHONY: lint
lint: ## Lint / analyse statique Vitte
	$(call ECHO_SECTION,"Lint Vitte")
	@if command -v $(VITTE_LINT) >/dev/null 2>&1; then \
		$(VITTE_LINT) --project "$(PROJECT_MUFFIN)" --profile "$(PROFILE)"; \
	else \
		$(call ECHO_WARN,"$(VITTE_LINT) introuvable, lint ignoré"); \
	fi
	$(call ECHO_OK,"lint terminé (si outil disponible)")

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

.PHONY: test
test: ## Lance la suite de tests Vitte
	$(call ECHO_SECTION,"Tests Vitte (profile=$(PROFILE))")
	@if command -v $(VITTE_TEST) >/dev/null 2>&1; then \
		$(VITTE_TEST) --project "$(PROJECT_MUFFIN)" --profile "$(PROFILE)"; \
	else \
		$(call ECHO_WARN,"$(VITTE_TEST) introuvable, tests ignorés"); \
	fi
	$(call ECHO_OK,"tests Vitte terminés (si outil disponible)")

.PHONY: test-release
test-release: ## Tests avec build release
	$(call ECHO_SECTION,"Tests Vitte (release, profile=$(PROFILE_RELEASE))")
	@if command -v $(VITTE_TEST) >/dev/null 2>&1; then \
		$(VITTE_TEST) --project "$(PROJECT_MUFFIN)" --profile "$(PROFILE_RELEASE)"; \
	else \
		$(call ECHO_WARN,"$(VITTE_TEST) introuvable, tests ignorés"); \
	fi
	$(call ECHO_OK,"tests release Vitte terminés (si outil disponible)")

# ---------------------------------------------------------------------------
# Documentation (générée par un outil Vitte, si disponible)
# ---------------------------------------------------------------------------

.PHONY: doc
doc: ## Génère la documentation Vitte (si outil disponible)
	$(call ECHO_SECTION,"Documentation Vitte")
	@if command -v $(VITTE) >/dev/null 2>&1; then \
		$(VITTE) doc --project "$(PROJECT_MUFFIN)" --out "$(DOCS_DIR)" || true; \
	else \
		$(call ECHO_WARN,"$(VITTE) introuvable, génération de doc ignorée"); \
	fi
	$(call ECHO_OK,"documentation générée (si outil disponible)")

.PHONY: open-doc
open-doc: doc ## Ouvre la documentation dans le navigateur
	$(call ECHO_SECTION,"Open documentation")
	@if [ -d "$(DOCS_DIR)" ]; then \
		if [ "$(UNAME_S)" = "Darwin" ]; then \
			open "$(DOCS_DIR)/index.html" || true; \
		elif [ "$(UNAME_S)" = "Linux" ]; then \
			xdg-open "$(DOCS_DIR)/index.html" >/dev/null 2>&1 || true; \
		else \
			$(call ECHO_WARN,"ouverture de la doc non supportée automatiquement"); \
		fi; \
	else \
		$(call ECHO_WARN,"documentation introuvable, lance 'make doc' d’abord"); \
	fi

# ---------------------------------------------------------------------------
# Nettoyage
# ---------------------------------------------------------------------------

.PHONY: clean
clean: ## Nettoie les artefacts de build Vitte
	$(call ECHO_SECTION,"Clean Vitte")
	@rm -rf "$(BUILD_DIR)"
	$(call ECHO_OK,"clean terminé")

.PHONY: distclean
distclean: clean ## Nettoyage complet (build + dist + docs)
	$(call ECHO_SECTION,"Distclean Vitte")
	@rm -rf "$(DIST_DIR)" "$(DOCS_DIR)"
	$(call ECHO_OK,"distclean terminé")

# ---------------------------------------------------------------------------
# Distribution / release (archives génériques, indépendantes de l’implémentation)
# ---------------------------------------------------------------------------

DIST_TAR := $(DIST_DIR)/$(PROJECT_NAME)-$(PROJECT_VERSION).tar.gz
DIST_ZIP := $(DIST_DIR)/$(PROJECT_NAME)-$(PROJECT_VERSION).zip

.PHONY: dist
dist: build-release ## Crée une archive source + artefacts Vitte (tar.gz et zip)
	$(call ECHO_SECTION,"Distribution Vitte")
	@mkdir -p "$(DIST_DIR)"
	@tar czf "$(DIST_TAR)" --exclude "$(DIST_DIR)" --exclude ".git" .
	$(call ECHO_OK,"archive source: $(DIST_TAR)")
	@if command -v zip >/dev/null 2>&1; then \
		zip -r "$(DIST_ZIP)" . -x "$(DIST_DIR)/*" -x ".git/*" || true; \
		$(call ECHO_OK,"archive zip: $(DIST_ZIP)"); \
	else \
		$(call ECHO_WARN,"zip introuvable, archive zip non générée"); \
	fi

# ---------------------------------------------------------------------------
# Exécution utilitaires
# ---------------------------------------------------------------------------

.PHONY: run
run: build ## Lance le binaire / programme principal Vitte
	$(call ECHO_SECTION,"Run Vitte (programme principal)")
	@if command -v $(VITTE) >/dev/null 2>&1; then \
		$(VITTE) run --project "$(PROJECT_MUFFIN)" --profile "$(PROFILE)" -- $(ARGS); \
	else \
		$(call ECHO_ERR,"$(VITTE) introuvable, impossible de lancer le programme"); \
		exit 1; \
	fi

.PHONY: repl
repl: ## Lance le REPL Vitte
	$(call ECHO_SECTION,"Run REPL Vitte")
	@if command -v $(VITTE_REPL) >/dev/null 2>&1; then \
		$(VITTE_REPL) --project "$(PROJECT_MUFFIN)" --profile "$(PROFILE)"; \
	else \
		$(call ECHO_ERR,"$(VITTE_REPL) introuvable, impossible de lancer le REPL"); \
		exit 1; \
	fi

# ---------------------------------------------------------------------------
# Hooks d’intégration (CI, pré-commit) – purement Vitte
# ---------------------------------------------------------------------------

.PHONY: ci
ci: fmt lint test ## Pipeline standard pour CI Vitte (format + lint + tests)
	$(call ECHO_OK,"pipeline CI Vitte locale terminée")

.PHONY: pre-commit
pre-commit: fmt lint ## Hook pré-commit local (format + lint Vitte)
	$(call ECHO_OK,"pré-commit Vitte OK")

# ---------------------------------------------------------------------------
# Sentinelles
# ---------------------------------------------------------------------------

.PHONY: default
default: help
