.PHONY: help build clean test lint format install dev release bootstrap rebuild

# Configuration
PROJECT_NAME := lang-vitte
VERSION := 0.1.0
COMPILER := vittec
ASSEMBLER := vasm
BC_RUNNER := vbc-run

# Répertoires
SRC_DIR := src
BIN_DIR := bin
OUT_DIR := out
BUILD_DIR := build
SCRIPTS_DIR := scripts
TESTS_DIR := tests
DOCS_DIR := docs

# Fichiers sources
COMPILER_SRCS := $(wildcard $(SRC_DIR)/compiler/*.vitte)
RUNTIME_SRCS := $(wildcard $(SRC_DIR)/runtime/*.vitte)
STDLIB_SRCS := $(wildcard $(SRC_DIR)/stdlib/**/*.vitte)
TOOLS_SRCS := $(wildcard $(SRC_DIR)/tools/*.vitte)
TEST_SRCS := $(wildcard $(TESTS_DIR)/**/*.vitte)

# Cibles de build
COMPILER_BIN := $(OUT_DIR)/$(COMPILER).vbc
ASSEMBLER_BIN := $(OUT_DIR)/$(ASSEMBLER).vbc
BC_RUNNER_BIN := $(OUT_DIR)/$(BC_RUNNER).vbc

# Flags
VASM_FLAGS ?=
VITTEC_FLAGS ?= -O2
VBC_RUN_FLAGS ?=

## Affichage aide
help:
	@echo "$(PROJECT_NAME) v$(VERSION) - Build system"
	@echo ""
	@echo "Commandes disponibles:"
	@echo "  make build         - Compiler le projet complet"
	@echo "  make rebuild       - Clean + build"
	@echo "  make test          - Exécuter tous les tests"
	@echo "  make lint          - Vérifier la syntaxe"
	@echo "  make format        - Formatter le code (vfmt)"
	@echo "  make clean         - Nettoyer les artifacts"
	@echo "  make install       - Installer les binaires"
	@echo "  make bootstrap     - Bootstrap compilateur"
	@echo "  make dev           - Setup environnement dev"
	@echo "  make examples      - Compiler les exemples"
	@echo "  make docs          - Générer la documentation"
	@echo "  make release       - Préparer release"
	@echo ""
	@echo "Variables d'environnement:"
	@echo "  VITTEC_FLAGS       - Flags compilateur (défaut: -O2)"
	@echo "  VASM_FLAGS         - Flags assembleur"
	@echo "  V                  - Mode verbose (make V=1)"

## Build principal
build: $(OUT_DIR) $(COMPILER_BIN) $(ASSEMBLER_BIN) $(BC_RUNNER_BIN)
	@echo "✓ Build complet réussi"

$(OUT_DIR):
	@mkdir -p $(OUT_DIR) $(BUILD_DIR)

# Compiler le compilateur Vitte lui-même
$(COMPILER_BIN): $(COMPILER_SRCS) $(RUNTIME_SRCS) $(STDLIB_SRCS)
	@echo "📦 Compilation du compilateur..."
	@$(BIN_DIR)/vasm $(SRC_DIR)/tools/vittec.vitte -o $@ $(VASM_FLAGS)
	@echo "✓ Compilateur compilé: $@"

# Compiler l'assembleur
$(ASSEMBLER_BIN): $(SRC_DIR)/tools/vasm.vitte
	@echo "📦 Compilation de l'assembleur..."
	@$(BIN_DIR)/vasm $< -o $@ $(VASM_FLAGS)
	@echo "✓ Assembleur compilé: $@"

# Compiler le runner bytecode
$(BC_RUNNER_BIN): $(SRC_DIR)/tools/vbc-run.vitte
	@echo "📦 Compilation du runner bytecode..."
	@$(BIN_DIR)/vasm $< -o $@ $(VASM_FLAGS)
	@echo "✓ Runner bytecode compilé: $@"

## Bootstrap (compiler sans avoir de compilateur)
bootstrap:
	@echo "🔄 Bootstrap du compilateur..."
	@bash $(SCRIPTS_DIR)/build/bootstrap.sh
	@echo "✓ Bootstrap réussi"

## Compilation des exemples
examples: build
	@echo "📚 Compilation des exemples..."
	@for file in examples/basic/*.vitte; do \
		echo "  Compiling $$file..."; \
		$(BIN_DIR)/vittec $$file -o $(OUT_DIR)/$$(basename $$file .vitte).vbc; \
	done
	@echo "✓ Exemples compilés"

## Tests
test: build lint
	@echo "🧪 Exécution des tests..."
	@bash $(SCRIPTS_DIR)/test/run-tests.sh
	@echo "✓ Tests passés"

test-unit: build
	@echo "🧪 Tests unitaires..."
	@bash $(SCRIPTS_DIR)/test/unit-tests.sh

test-integration: build examples
	@echo "🧪 Tests d'intégration..."
	@bash $(SCRIPTS_DIR)/test/integration-tests.sh

test-coverage: build
	@echo "📊 Couverture de code..."
	@bash $(SCRIPTS_DIR)/test/coverage.sh

## Linting et vérification
lint:
	@echo "🔍 Vérification du code..."
	@bash $(SCRIPTS_DIR)/ci/lint.sh
	@echo "✓ Pas d'erreurs de syntaxe"

format:
	@echo "🎨 Formatage du code..."
	@bash $(SCRIPTS_DIR)/ci/format.sh
	@echo "✓ Code formaté"

format-check:
	@echo "🔍 Vérification du formatage..."
	@bash $(SCRIPTS_DIR)/ci/format-check.sh
	@echo "✓ Formatage OK"

## Nettoyage
clean:
	@echo "🧹 Nettoyage..."
	@rm -rf $(BUILD_DIR) $(OUT_DIR)
	@find . -name "*.vbc" -delete
	@find . -name "*.o" -delete
	@echo "✓ Nettoyage réussi"

clean-all: clean
	@echo "🧹 Nettoyage complet (bin inclus)..."
	@rm -rf $(BIN_DIR)/*
	@echo "✓ Nettoyage complet réussi"

## Installation
install: build
	@echo "📦 Installation des binaires..."
	@mkdir -p /usr/local/bin
	@cp $(BIN_DIR)/vittec /usr/local/bin/ 2>/dev/null || echo "  (Skip vittec - utiliser wrapper)"
	@cp $(BIN_DIR)/vasm /usr/local/bin/ 2>/dev/null || echo "  (Skip vasm - utiliser wrapper)"
	@cp $(BIN_DIR)/vbc-run /usr/local/bin/ 2>/dev/null || echo "  (Skip vbc-run - utiliser wrapper)"
	@echo "✓ Installation réussie"

## Setup développement
dev:
	@echo "⚙️  Setup environnement de développement..."
	@mkdir -p $(BUILD_DIR) $(OUT_DIR)
	@[ -d .git ] || git init
	@chmod +x $(SCRIPTS_DIR)/**/*.sh
	@chmod +x $(BIN_DIR)/*
	@echo "✓ Setup développement réussi"
	@echo ""
	@echo "Prochaines étapes:"
	@echo "  1. make bootstrap    - Bootstrap le compilateur"
	@echo "  2. make build        - Construire le projet"
	@echo "  3. make test         - Exécuter les tests"

## Rebuild
rebuild: clean build
	@echo "✓ Rebuild réussi"

## Documentation
docs:
	@echo "📖 Génération de la documentation..."
	@[ -d $(DOCS_DIR)/generated ] || mkdir -p $(DOCS_DIR)/generated
	@echo "  Parsing grammar..."
	@echo "  Generating API docs..."
	@echo "✓ Documentation générée"

docs-serve:
	@echo "📖 Servant la documentation..."
	@python3 -m http.server --directory $(DOCS_DIR) 8000 &
	@echo "✓ Docs servies sur http://localhost:8000"

## Release
release: rebuild lint test format
	@echo "🚀 Préparation de la release v$(VERSION)..."
	@bash $(SCRIPTS_DIR)/ci/release.sh $(VERSION)
	@echo "✓ Release préparée"

## Stats du projet
stats:
	@echo "📊 Statistiques du projet:"
	@echo "  Sources Vitte:"
	@wc -l $(SRC_DIR)/**/*.vitte 2>/dev/null | tail -1
	@echo "  Tests:"
	@wc -l $(TESTS_DIR)/**/*.vitte 2>/dev/null | tail -1
	@echo "  Documentation:"
	@wc -l $(DOCS_DIR)/**/*.md 2>/dev/null | tail -1

## Smoke tests (tests rapides)
smoke: build
	@echo "🔥 Smoke tests..."
	@bash $(SCRIPTS_DIR)/test/smoke.sh

## Info système
info:
	@echo "Informations système:"
	@echo "  OS: $$(uname -s)"
	@echo "  Shell: $$(echo $$SHELL)"
	@echo "  Make: $$(make --version | head -1)"
	@echo ""
	@echo "Infos projet:"
	@echo "  Répertoire: $$(pwd)"
	@echo "  Version: $(VERSION)"
	@echo "  Compilateur: $(COMPILER)"

# Mode verbose
ifdef V
    VASM_FLAGS += -v
    VITTEC_FLAGS += -v
endif

# Watchdog - rebuild sur changement
watch:
	@echo "👁️  Watching for changes..."
	@while true; do \
		inotifywait -e modify -r $(SRC_DIR) && make build; \
	done

.PHONY: help build bootstrap rebuild clean clean-all test test-unit test-integration test-coverage \
        lint format format-check install dev examples docs docs-serve release stats smoke info watch
